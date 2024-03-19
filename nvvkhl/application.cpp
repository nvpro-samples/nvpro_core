/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


/**
/class nvvkhl::Application

    The Application is basically a small modification of the ImGui example for Vulkan.
    Because we support multiple viewports, duplicating the code would be not necessary 
    and the code is very well explained. 

    Worth notice
    - The Application is a singleton, and the main loop is inside the run() function.
    - The Application is the owner of the elements, and it will call the onRender, onUIRender, onUIMenu
      for each element that is connected to it.
    - The Application is the owner of the Vulkan context, and it will create the surface and window.
    - The Application is the owner of the ImGui context, and it will create the dockspace and the main menu.

    - ::init() : will create the GLFW window, call nvvk::context for the creation of the 
               Vulkan context, initialize ImGui , create the surface and window (::setupVulkanWindow)
    
    - ::shutdown() : the oposite of init

    - ::run() : while running, render the frame and present the frame. Check for resize, minimize window 
                and other changes. In the loop, it will call some functions for each 'element' that is connected.
                onUIRender, onUIMenu, onRender. See IApplication for details.
*/


#include <iostream>
#include <filesystem>
#include <imgui_internal.h>
#include <implot.h>

#include "application.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui/imgui_camera_widget.h"
#include "imgui/imgui_helper.h"
#include "imgui/imgui_icon.h"
#include "nvpsystem.hpp"
#include "nvvk/context_vk.hpp"
#include "nvvk/error_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "perproject_globals.hpp"


#ifdef LINUX
#include <unistd.h>
#endif

// Embedded font
#include "Roboto-Regular.h"

// GLFW
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// To save images
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

// Static
uint32_t                                        nvvkhl::Application::m_currentFrameIndex{0};
std::vector<std::vector<std::function<void()>>> nvvkhl::Application::m_resourceFreeQueue;

// Forward declaration
void ImplVulkanH_CreateOrResizeWindow(VkInstance                   instance,
                                      VkPhysicalDevice             physical_device,
                                      VkDevice                     device,
                                      ImGui_ImplVulkanH_Window*    wnd,
                                      uint32_t                     queue_family,
                                      const VkAllocationCallbacks* allocator,
                                      int                          w,
                                      int                          h,
                                      uint32_t                     min_image_count);


// GLFW Callback functions
static void onErrorCallback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void checkVkResult(VkResult err)
{
  if(err == 0)
  {
    return;
  }
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if(err < 0)
  {
    abort();
  }
}

void dropCb(GLFWwindow* window, int count, const char** paths)
{
  auto* app = static_cast<nvvkhl::Application*>(glfwGetWindowUserPointer(window));
  for(int i = 0; i < count; i++)
  {
    app->onFileDrop(paths[i]);
  }
}


nvvkhl::Application::Application(ApplicationCreateInfo& info)
{
  init(info);
}


nvvkhl::Application::~Application()
{
  shutdown();
}

void nvvkhl::Application::init(ApplicationCreateInfo& info)
{
  auto path_log = std::filesystem::path(NVPSystem::exePath())
                  / std::filesystem::path(std::string("log_") + getProjectName() + std::string(".txt"));
  auto path_ini = std::filesystem::path(NVPSystem::exePath()) / std::filesystem::path(getProjectName() + std::string(".ini"));

  m_clearColor            = info.clearColor;
  m_dockSetup             = info.dockSetup;
  m_hasUndockableViewport = info.hasUndockableViewport;

  // setup some basic things for the sample, logging file for example
  nvprintSetLogFileName(path_log.string().c_str());

  m_mainWindowData = std::make_unique<ImGui_ImplVulkanH_Window>();

  m_useMenubar     = info.useMenu;
  m_useDockMenubar = info.useDockMenu;
  m_vsyncWanted    = info.vSync;

  // Setup GLFW window
  glfwSetErrorCallback(onErrorCallback);
  if(glfwInit() == 0)
  {
    std::cerr << "Could not initialize GLFW!\n";
    return;
  }

  const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

  // If not defined, set the window to 80% of the screen size
  if(info.width <= 0 || info.height <= 0)
  {
    info.width  = static_cast<int>(static_cast<float>(mode->width) * 0.8F);
    info.height = static_cast<int>(static_cast<float>(mode->height) * 0.8F);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_windowHandle = glfwCreateWindow(info.width, info.height, info.name.c_str(), nullptr, nullptr);

  glfwSetWindowPos(m_windowHandle, static_cast<int>((mode->width - info.width) * 0.5),
                   static_cast<int>((mode->height - info.height) * 0.5));

  // Set the Drop callback
  glfwSetWindowUserPointer(m_windowHandle, this);
  glfwSetDropCallback(m_windowHandle, &dropCb);

  // Setup Vulkan
  if(glfwVulkanSupported() == 0)
  {
    std::cerr << "GLFW: Vulkan not supported!\n";
    return;
  }

  // Adding required extensions
  nvvk::ContextCreateInfo& vk_setup         = info.vkSetup;
  uint32_t                 extensions_count = 0;
  const char**             extensions       = glfwGetRequiredInstanceExtensions(&extensions_count);
  for(uint32_t i = 0; i < extensions_count; i++)
  {
    vk_setup.instanceExtensions.emplace_back(extensions[i]);
  }
  vk_setup.deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  vk_setup.instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // Vulkan context creation
  m_context = std::make_shared<nvvk::Context>();
  for(auto& dbg_msg : info.ignoreDbgMessages)
  {
    m_context->ignoreDebugMessage(dbg_msg);  // Turn-off messages
  }
  m_context->init(vk_setup);


  createDescriptorPool();

  VkCommandPoolCreateInfo pool_create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  vkCreateCommandPool(m_context->m_device, &pool_create_info, m_allocator, &m_cmdPool);

  VkPipelineCacheCreateInfo pipeline_cache_info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
  vkCreatePipelineCache(m_context->m_device, &pipeline_cache_info, m_allocator, &m_pipelineCache);

  // Create Window Surface
  VkSurfaceKHR surface = nullptr;
  NVVK_CHECK(glfwCreateWindowSurface(m_context->m_instance, m_windowHandle, m_allocator, &surface));

  // Create framebuffers
  int w{0};
  int h{0};
  glfwGetFramebufferSize(m_windowHandle, &w, &h);
  ImGui_ImplVulkanH_Window* wd = m_mainWindowData.get();
  setupVulkanWindow(surface, w, h);

  resetFreeQueue(wd->ImageCount);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  m_iniFilename  = path_ini.string();
  io.IniFilename = m_iniFilename.c_str();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
  io.ConfigWindowsMoveFromTitleBarOnly = true;

  // Setup Dear ImGui style
  ImGuiH::setStyle();

  // Load default font
  const float  high_dpi_scale = ImGuiH::getDPIScale();
  ImFontConfig font_config;
  font_config.FontDataOwnedByAtlas = false;
  io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&g_Roboto_Regular[0], sizeof(g_Roboto_Regular),
                                                  14.0F * high_dpi_scale, &font_config);

  // Add icon font
  ImGuiH::addIconicFont();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(m_windowHandle, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = m_context->m_instance;
  init_info.PhysicalDevice            = m_context->m_physicalDevice;
  init_info.Device                    = m_context->m_device;
  init_info.QueueFamily               = m_context->m_queueGCT.familyIndex;
  init_info.Queue                     = m_context->m_queueGCT.queue;
  init_info.PipelineCache             = m_pipelineCache;
  init_info.DescriptorPool            = m_descriptorPool;
  init_info.RenderPass                = wd->RenderPass;
  init_info.Subpass                   = 0;
  init_info.MinImageCount             = m_minImageCount;
  init_info.ImageCount                = wd->ImageCount;
  init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator                 = m_allocator;
  init_info.CheckVkResultFn           = checkVkResult;
  ImGui_ImplVulkan_Init(&init_info);

  // Read camera setting
  ImGuiH::SetCameraJsonFile(getProjectName());

  // Implot
  ImPlot::CreateContext();
}

//--------------------------------------------------------------------------------------------------
// To call on exit
//
void nvvkhl::Application::shutdown()
{
  for(auto& e : m_elements)
  {
    e->onDetach();
  }

  // Cleanup
  vkDeviceWaitIdle(m_context->m_device);

  // Free all resources
  resetFreeQueue(0);

  if(ImPlot::GetCurrentContext() != nullptr)
  {
    ImPlot::DestroyContext();
  }

  if(ImGui::GetCurrentContext() != nullptr)
  {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  // Destroying all attached application elements
  m_elements.clear();

  // Cleanup Vulkan Window
  ImGui_ImplVulkanH_DestroyWindow(m_context->m_instance, m_context->m_device, m_mainWindowData.get(), m_allocator);

  // Vulkan cleanup
  vkDestroyPipelineCache(m_context->m_device, m_pipelineCache, m_allocator);
  vkDestroyCommandPool(m_context->m_device, m_cmdPool, m_allocator);
  vkDestroyDescriptorPool(m_context->m_device, m_descriptorPool, m_allocator);

  m_context->deinit();

  // Glfw cleanup
  glfwDestroyWindow(m_windowHandle);
  glfwTerminate();
}


void nvvkhl::Application::setupVulkanWindow(VkSurfaceKHR surface, int width, int height)
{
  ImGui_ImplVulkanH_Window* wd = m_mainWindowData.get();
  wd->Surface                  = surface;

  m_windowSize.width  = width;
  m_windowSize.height = height;

  // Check for WSI support
  VkBool32 res = VK_FALSE;
  NVVK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_context->m_physicalDevice, m_context->m_queueGCT.familyIndex, wd->Surface, &res));
  if(res != VK_TRUE)
  {
    fprintf(stderr, "Error no WSI support on physical device\n");
    exit(-1);
  }

  // Select Surface Format
  const std::array<VkFormat, 4> request_surface_image_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
                                                                VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
  const VkColorSpaceKHR         request_surface_color_space  = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  wd->SurfaceFormat =
      ImGui_ImplVulkanH_SelectSurfaceFormat(m_context->m_physicalDevice, wd->Surface, request_surface_image_format.data(),
                                            static_cast<int>(request_surface_image_format.size()), request_surface_color_space);

  // Select Present Mode
  setPresentMode(m_context->m_physicalDevice, wd);

  // Create SwapChain, RenderPass, Framebuffer, etc.
  ImplVulkanH_CreateOrResizeWindow(m_context->m_instance, m_context->m_physicalDevice, m_context->m_device, wd,
                                   m_context->m_queueGCT.familyIndex, m_allocator, width, height, m_minImageCount);
}


void nvvkhl::Application::run()
{
  m_running = true;

  ImGui_ImplVulkanH_Window* wd          = m_mainWindowData.get();
  ImVec4                    clear_color = ImVec4(0.0F, 0.0F, 0.0F, 1.00F);
  ImGuiIO&                  io          = ImGui::GetIO();

  // Main loop
  while((glfwWindowShouldClose(m_windowHandle) == 0) && m_running)
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // Resize swap chain?
    if(m_swapChainRebuild || m_vsyncSet != m_vsyncWanted)
    {
      int width{0};
      int height{0};
      glfwGetFramebufferSize(m_windowHandle, &width, &height);
      if(width > 0 && height > 0)
      {
        setPresentMode(m_context->m_physicalDevice, wd);

        ImGui_ImplVulkan_SetMinImageCount(m_minImageCount);
        ImplVulkanH_CreateOrResizeWindow(m_context->m_instance, m_context->m_physicalDevice, m_context->m_device,
                                         m_mainWindowData.get(), m_context->m_queueGCT.familyIndex, m_allocator, width,
                                         height, m_minImageCount);
        resetFreeQueue(wd->ImageCount);
        m_mainWindowData->FrameIndex = 0;
        m_swapChainRebuild           = false;
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Docking
    {
      createDock();

      // Adding menu
      if(m_useMenubar)
      {
        if(ImGui::BeginMainMenuBar())
        {
          for(auto& e : m_elements)
          {
            e->onUIMenu();
          }

          ImGui::EndMainMenuBar();
        }
      }

      // Setting up the viewport and getting information
      ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
      if(m_hasUndockableViewport || viewportWindow)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, m_clearColor);

        ImGui::Begin("Viewport");
        ImVec2 viewport_size         = ImGui::GetContentRegionAvail();
        bool   viewport_size_changed = (static_cast<uint32_t>(viewport_size.x) != m_viewportSize.width
                                      || static_cast<uint32_t>(viewport_size.y) != m_viewportSize.height);
        bool   viewport_valid        = viewport_size.x > 0 && viewport_size.y > 0;
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Callback if the viewport changed sizes
        if(viewport_size_changed && viewport_valid)
        {
          m_viewportSize.width  = static_cast<uint32_t>(viewport_size.x);
          m_viewportSize.height = static_cast<uint32_t>(viewport_size.y);
          for(auto& e : m_elements)
          {
            e->onResize(m_viewportSize.width, m_viewportSize.height);
          }
        }
      }

      // Call the implementation of the UI rendering
      for(auto& e : m_elements)
      {
        e->onUIRender();
      }
    }

    // Rendering
    ImGui::Render();
    ImDrawData* main_draw_data      = ImGui::GetDrawData();
    const bool  main_is_minimized   = (main_draw_data->DisplaySize.x <= 0.0F || main_draw_data->DisplaySize.y <= 0.0F);
    wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    wd->ClearValue.color.float32[3] = clear_color.w;
    if(!main_is_minimized)
    {
      frameRender();
    }

    // Update and Render additional Platform Windows (floating windows)
    // See: ImGui_ImplVulkan_InitPlatformInterface()
    if((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }

    if(m_screenShotRequested)
    {
      m_screenShotRequested = false;
      vkDeviceWaitIdle(m_context->m_device);
      saveScreenShot(m_screenShotFilename, m_screenShotQuality);
    }

    // Present Main Platform Window
    if(!main_is_minimized)
    {
      framePresent();
    }
  }
}


void nvvkhl::Application::createDock() const
{
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  // Keeping the unique ID of the dock space
  ImGuiID dockspace_id = ImGui::GetID("DockSpace");

  // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
  // because it would be confusing to have two docking targets within each others.
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
  if(m_useDockMenubar)
  {
    window_flags |= ImGuiWindowFlags_MenuBar;
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
  // and handle the pass-thru hole, so we ask Begin() to not render a background.
  if((dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0)
  {
    window_flags |= ImGuiWindowFlags_NoBackground;
  }

  // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
  // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
  // all active windows docked into it will lose their parent and become undocked.
  // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
  // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
  ImGui::Begin("DockSpace NVVKHL", nullptr, window_flags);
  ImGui::PopStyleVar();

  ImGui::PopStyleVar(2);

  dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
  if(m_hasUndockableViewport)
    dockspace_flags |= ImGuiDockNodeFlags_NoDockingOverCentralNode;

  // Building the splitting of the dock space is done only once
  if(ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
  {
    ImGuiID dock_main_id = dockspace_id;
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
    ImGui::DockBuilderDockWindow("Viewport", dockspace_id);  // Center

    // Disable tab bar for custom toolbar
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_main_id);
    if(m_hasUndockableViewport)
      node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

    if(m_dockSetup)
    {
      // This override allow to create the layout of windows by default.
      // All windows in the application must be named here, otherwise they will appear un-docked
      m_dockSetup(dockspace_id);
    }
    else
    {  // Default with a window named "Settings" on the right
      ImGuiID id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2F, nullptr, &dock_main_id);
      ImGui::DockBuilderDockWindow("Settings", id_right);
    }

    ImGui::DockBuilderFinish(dock_main_id);
  }

  // Submit the DockSpace
  ImGuiIO& io = ImGui::GetIO();
  if((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0)
  {
    ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), dockspace_flags);
  }
  ImGui::End();
}

void nvvkhl::Application::close()
{
  m_running = false;
}

void nvvkhl::Application::addElement(const std::shared_ptr<IAppElement>& layer)
{
  m_elements.emplace_back(layer);
  layer->onAttach(this);
}

void nvvkhl::Application::submitResourceFree(std::function<void()>&& func)
{
  if(m_currentFrameIndex < m_resourceFreeQueue.size())
  {
    m_resourceFreeQueue[m_currentFrameIndex].emplace_back(func);
  }
  else
  {
    func();
  }
}

void nvvkhl::Application::frameRender()
{
  ImGui_ImplVulkanH_Window* wd        = m_mainWindowData.get();
  ImDrawData*               draw_data = ImGui::GetDrawData();
  m_waitSemaphores.clear();
  m_signalSemaphores.clear();
  m_commandBuffers.clear();

  VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VkResult    err = vkAcquireNextImageKHR(m_context->m_device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore,
                                          VK_NULL_HANDLE, &wd->FrameIndex);
  if(err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
  {
    m_swapChainRebuild = true;
    return;
  }
  NVVK_CHECK(err);

  m_currentFrameIndex = (m_currentFrameIndex + 1) % wd->ImageCount;

  ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
  {
    NVVK_CHECK(vkWaitForFences(m_context->m_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));  // wait indefinitely instead of periodically checking
    NVVK_CHECK(vkResetFences(m_context->m_device, 1, &fd->Fence));
  }
  {
    // Free resources in queue
    for(auto& func : m_resourceFreeQueue[m_currentFrameIndex])
    {
      func();
    }
    m_resourceFreeQueue[m_currentFrameIndex].clear();
  }
  {
    NVVK_CHECK(vkResetCommandPool(m_context->m_device, fd->CommandPool, 0));
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    NVVK_CHECK(vkBeginCommandBuffer(fd->CommandBuffer, &info));
  }

  for(auto& e : m_elements)
  {
    e->onRender(fd->CommandBuffer);
  }

  {
    VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    info.renderPass               = wd->RenderPass;
    info.framebuffer              = fd->Framebuffer;
    info.renderArea.extent.width  = wd->Width;
    info.renderArea.extent.height = wd->Height;
    info.clearValueCount          = 1;
    info.pClearValues             = &wd->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  {
    // Record Dear Imgui primitives into command buffer
    VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, "ImGui_ImplVulkan_RenderDrawData", {1.0f, 1.0f, 1.0f, 1.0f}};
    vkCmdBeginDebugUtilsLabelEXT(fd->CommandBuffer, &s);
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);
    vkCmdEndDebugUtilsLabelEXT(fd->CommandBuffer);
  }

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkCommandBufferSubmitInfo cmdBufInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdBufInfo.commandBuffer = fd->CommandBuffer;
    m_commandBuffers.emplace_back(cmdBufInfo);

    VkSemaphoreSubmitInfo signalSemaphore{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signalSemaphore.semaphore = render_complete_semaphore;
    signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_signalSemaphores.emplace_back(signalSemaphore);

    VkSemaphoreSubmitInfo waitSemaphore{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    waitSemaphore.semaphore = image_acquired_semaphore;
    waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_waitSemaphores.emplace_back(waitSemaphore);

    VkSubmitInfo2 submits{VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
    submits.commandBufferInfoCount   = (uint32_t)m_commandBuffers.size();
    submits.pCommandBufferInfos      = m_commandBuffers.data();
    submits.waitSemaphoreInfoCount   = (uint32_t)m_waitSemaphores.size();
    submits.pWaitSemaphoreInfos      = m_waitSemaphores.data();
    submits.signalSemaphoreInfoCount = (uint32_t)m_signalSemaphores.size();
    submits.pSignalSemaphoreInfos    = m_signalSemaphores.data();

    NVVK_CHECK(vkEndCommandBuffer(fd->CommandBuffer));
    NVVK_CHECK(vkQueueSubmit2(m_context->m_queueGCT.queue, 1, &submits, fd->Fence));
  }
}

void nvvkhl::Application::addWaitSemaphore(const VkSemaphoreSubmitInfoKHR& wait)
{
  m_waitSemaphores.push_back(wait);
}
void nvvkhl::Application::addSignalSemaphore(const VkSemaphoreSubmitInfoKHR& signal)
{
  m_signalSemaphores.push_back(signal);
}
void nvvkhl::Application::prependCommandBuffer(const VkCommandBufferSubmitInfoKHR& cmd)
{
  m_commandBuffers.push_back(cmd);
}

void nvvkhl::Application::framePresent()
{
  ImGui_ImplVulkanH_Window* wd = m_mainWindowData.get();

  if(m_swapChainRebuild)
  {
    return;
  }

  VkSemaphore      render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores    = &render_complete_semaphore;
  info.swapchainCount     = 1;
  info.pSwapchains        = &wd->Swapchain;
  info.pImageIndices      = &wd->FrameIndex;
  VkResult err            = vkQueuePresentKHR(m_context->m_queueGCT.queue, &info);
  if(err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
  {
    m_swapChainRebuild = true;
    return;
  }
  checkVkResult(err);
  wd->FrameIndex     = (wd->FrameIndex + 1) % wd->ImageCount;          // This is for the next vkWaitForFences()
  wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;  // Now we can use the next set of semaphores
}


bool nvvkhl::Application::isVsync() const
{
  return m_vsyncSet;
}

void nvvkhl::Application::setVsync(bool v)
{
  m_vsyncWanted = v;
}


void nvvkhl::Application::setPresentMode(VkPhysicalDevice physicalDevice, ImGui_ImplVulkanH_Window* wd)
{
  m_vsyncSet = m_vsyncWanted;

  std::vector<VkPresentModeKHR> present_modes;
  if(m_vsyncSet)
  {
    present_modes = {VK_PRESENT_MODE_FIFO_KHR};
  }
  else
  {
    present_modes = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
  }

  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physicalDevice, wd->Surface, present_modes.data(),
                                                        static_cast<int>(present_modes.size()));
}

void nvvkhl::Application::createDescriptorPool()
{
  std::vector<VkDescriptorPoolSize> pool_sizes = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                                  {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                                  {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                                  {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                                  {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                                  {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                                  {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                                  {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                                  {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                                  {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                                  {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets       = 1000 * static_cast<uint32_t>(pool_sizes.size());
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes    = pool_sizes.data();
  NVVK_CHECK(vkCreateDescriptorPool(m_context->m_device, &pool_info, nullptr, &m_descriptorPool));
}

void nvvkhl::Application::resetFreeQueue(uint32_t size)
{
  vkDeviceWaitIdle(m_context->m_device);

  for(auto& queue : m_resourceFreeQueue)
  {
    // Free resources in queue
    for(auto& func : queue)
    {
      func();
    }
    queue.clear();
  }
  m_resourceFreeQueue.clear();
  m_resourceFreeQueue.resize(size);
}

// Set the viewport using with the size of the "Viewport" window
void nvvkhl::Application::setViewport(const VkCommandBuffer& cmd)
{
  VkViewport viewport{0.0F, 0.0F, static_cast<float>(m_viewportSize.width), static_cast<float>(m_viewportSize.height),
                      0.0F, 1.0F};
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, {m_viewportSize.width, m_viewportSize.height}};
  vkCmdSetScissor(cmd, 0, 1, &scissor);
}

// Create a temporary command buffer
VkCommandBuffer nvvkhl::Application::createTempCmdBuffer()
{
  VkCommandBufferAllocateInfo allocate_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocate_info.commandBufferCount = 1;
  allocate_info.commandPool        = m_cmdPool;
  allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  VkCommandBuffer cmd_buffer       = nullptr;
  vkAllocateCommandBuffers(m_context->m_device, &allocate_info, &cmd_buffer);

  VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd_buffer, &begin_info);
  return cmd_buffer;
}

// Submit the temporary command buffer
void nvvkhl::Application::submitAndWaitTempCmdBuffer(VkCommandBuffer cmd)
{
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &cmd;
  vkQueueSubmit(m_context->m_queueGCT.queue, 1, &submit_info, {});
  vkQueueWaitIdle(m_context->m_queueGCT.queue);
  vkFreeCommandBuffers(m_context->m_device, m_cmdPool, 1, &cmd);
}

void nvvkhl::Application::onFileDrop(const char* filename)
{
  for(auto& e : m_elements)
  {
    e->onFileDrop(filename);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// Screenshot
//
//////////////////////////////////////////////////////////////////////////

// Convert a tiled image to RGBA8 linear
void imageToRgba8Linear(VkCommandBuffer  cmd,
                        VkDevice         device,
                        VkPhysicalDevice physicalDevice,
                        VkImage          srcImage,
                        VkExtent2D       size,
                        VkImage&         dstImage,
                        VkDeviceMemory&  dstImageMemory)
{
  // Find the memory type index for the memory
  auto getMemoryType = [&](uint32_t typeBits, const VkMemoryPropertyFlags& properties) {
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &prop);
    for(uint32_t i = 0; i < prop.memoryTypeCount; i++)
    {
      if(((typeBits & (1 << i)) > 0) && (prop.memoryTypes[i].propertyFlags & properties) == properties)
        return i;
    }
    return ~0u;  // Unable to find memoryType
  };


  // Create the linear tiled destination image to copy to and to read the memory from
  VkImageCreateInfo imageCreateCI = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateCI.imageType         = VK_IMAGE_TYPE_2D;
  imageCreateCI.format            = VK_FORMAT_R8G8B8A8_UNORM;
  imageCreateCI.extent.width      = size.width;
  imageCreateCI.extent.height     = size.height;
  imageCreateCI.extent.depth      = 1;
  imageCreateCI.arrayLayers       = 1;
  imageCreateCI.mipLevels         = 1;
  imageCreateCI.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateCI.samples           = VK_SAMPLE_COUNT_1_BIT;
  imageCreateCI.tiling            = VK_IMAGE_TILING_LINEAR;
  imageCreateCI.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  NVVK_CHECK(vkCreateImage(device, &imageCreateCI, nullptr, &dstImage));

  // Create memory for the image
  // We want host visible and coherent memory to be able to map it and write to it directly
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
  VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memAllocInfo.allocationSize       = memRequirements.size;
  memAllocInfo.memoryTypeIndex =
      getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  NVVK_CHECK(vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImageMemory));
  NVVK_CHECK(vkBindImageMemory(device, dstImage, dstImageMemory, 0));

  nvvk::cmdBarrierImageLayout(cmd, srcImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  nvvk::cmdBarrierImageLayout(cmd, dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Do the actual blit from the swapchain image to our host visible destination image
  // The Blit allow to convert the image from VK_FORMAT_B8G8R8A8_UNORM to VK_FORMAT_R8G8B8A8_UNORM automatically
  VkOffset3D  blitSize = {int32_t(size.width), int32_t(size.height), 1};
  VkImageBlit imageBlitRegion{};
  imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBlitRegion.srcSubresource.layerCount = 1;
  imageBlitRegion.srcOffsets[1]             = blitSize;
  imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBlitRegion.dstSubresource.layerCount = 1;
  imageBlitRegion.dstOffsets[1]             = blitSize;
  vkCmdBlitImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                 &imageBlitRegion, VK_FILTER_NEAREST);

  nvvk::cmdBarrierImageLayout(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  nvvk::cmdBarrierImageLayout(cmd, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
}

// Record that a screenshot is requested, and will be saved at the end of the frame (saveScreenShot)
void nvvkhl::Application::screenShot(const std::string& filename, int quality)
{
  m_screenShotRequested = true;
  m_screenShotFilename  = filename;
  m_screenShotQuality   = quality;
}

// Save the current swapchain image to a file
void nvvkhl::Application::saveScreenShot(const std::string& filename, int quality)
{
  ImGui_ImplVulkanH_Window* wd       = m_mainWindowData.get();
  VkExtent2D                size     = {uint32_t(wd->Width), uint32_t(wd->Height)};
  VkDevice                  device   = m_context->m_device;
  VkImage                   srcImage = wd->Frames[wd->FrameIndex].Backbuffer;
  VkImage                   dstImage;
  VkDeviceMemory            dstImageMemory;

  VkCommandBuffer cmd = createTempCmdBuffer();
  imageToRgba8Linear(cmd, device, m_context->m_physicalDevice, srcImage, size, dstImage, dstImageMemory);
  submitAndWaitTempCmdBuffer(cmd);

  // Get layout of the image (including offset and row pitch)
  VkImageSubresource  subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
  VkSubresourceLayout subResourceLayout;
  vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

  // Map image memory so we can start copying from it
  const char* data;
  vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
  data += subResourceLayout.offset;

  // Copy the data and adjust for the row pitch
  std::vector<uint8_t> pixels(size.width * size.height * 4);
  for (uint32_t y = 0; y < size.height; y++)
  {
    memcpy(pixels.data() + y * size.width * 4, data, size.width * 4);
    data += subResourceLayout.rowPitch;
  }

  std::filesystem::path path      = filename;
  std::string           extension = path.extension().string();

  // Check the extension and perform actions accordingly
  if(extension == ".png")
  {
    stbi_write_png(filename.c_str(), size.width, size.height, 4, pixels.data(), size.width * 4);
  }
  else if(extension == ".jpg" || extension == ".jpeg")
  {
    stbi_write_jpg(filename.c_str(), size.width, size.height, 4, pixels.data(), quality);
  }
  else if(extension == ".bmp")
  {
    stbi_write_bmp(filename.c_str(), size.width, size.height, 4, pixels.data());
  }
  else
  {
    LOGW("Screenshot: unknown file extension, saving as PNG\n");
    stbi_write_png(filename.c_str(), size.width, size.height, 4, data, size.width * 4);
  }

  LOGI("Screenshot saved to %s\n", filename.c_str());

  // Clean up resources
  vkUnmapMemory(device, dstImageMemory);
  vkFreeMemory(device, dstImageMemory, nullptr);
  vkDestroyImage(device, dstImage, nullptr);
}

//////////////////////////////////////////////////////////////////////////
//
// Vulkan Helper
// This section is a copy of the ImGui_ImplVulkanH_XXX functions
// with modifications to be used in the Application class
//
//////////////////////////////////////////////////////////////////////////

void ImplVulkanH_CreateWindowCommandBuffers(VkPhysicalDevice             physical_device,
                                            VkDevice                     device,
                                            ImGui_ImplVulkanH_Window*    wd,
                                            uint32_t                     queue_family,
                                            const VkAllocationCallbacks* allocator)
{
  IM_ASSERT(physical_device != VK_NULL_HANDLE && device != VK_NULL_HANDLE);
  IM_UNUSED(physical_device);

  // Create Command Buffers
  for(uint32_t i = 0; i < wd->ImageCount; i++)
  {
    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[i];
    {
      VkCommandPoolCreateInfo info = {};
      info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.flags                   = 0;  //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      info.queueFamilyIndex        = queue_family;
      NVVK_CHECK(vkCreateCommandPool(device, &info, allocator, &fd->CommandPool));
    }
    {
      VkCommandBufferAllocateInfo info = {};
      info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool                 = fd->CommandPool;
      info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      info.commandBufferCount          = 1;
      NVVK_CHECK(vkAllocateCommandBuffers(device, &info, &fd->CommandBuffer));
    }
    {
      VkFenceCreateInfo info = {};
      info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
      NVVK_CHECK(vkCreateFence(device, &info, allocator, &fd->Fence));
    }
  }

  for(uint32_t i = 0; i < wd->SemaphoreCount; i++)
  {
    ImGui_ImplVulkanH_FrameSemaphores* fsd = &wd->FrameSemaphores[i];
    {
      VkSemaphoreCreateInfo info = {};
      info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      NVVK_CHECK(vkCreateSemaphore(device, &info, allocator, &fsd->ImageAcquiredSemaphore));
      NVVK_CHECK(vkCreateSemaphore(device, &info, allocator, &fsd->RenderCompleteSemaphore));
    }
  }
}

void ImplVulkanH_DestroyFrame(VkDevice device, ImGui_ImplVulkanH_Frame* fd, const VkAllocationCallbacks* allocator)
{
  vkDestroyFence(device, fd->Fence, allocator);
  vkFreeCommandBuffers(device, fd->CommandPool, 1, &fd->CommandBuffer);
  vkDestroyCommandPool(device, fd->CommandPool, allocator);
  fd->Fence         = VK_NULL_HANDLE;
  fd->CommandBuffer = VK_NULL_HANDLE;
  fd->CommandPool   = VK_NULL_HANDLE;

  vkDestroyImageView(device, fd->BackbufferView, allocator);
  vkDestroyFramebuffer(device, fd->Framebuffer, allocator);
}


void ImplVulkanH_DestroyFrameSemaphores(VkDevice device, ImGui_ImplVulkanH_FrameSemaphores* fsd, const VkAllocationCallbacks* allocator)
{
  vkDestroySemaphore(device, fsd->ImageAcquiredSemaphore, allocator);
  vkDestroySemaphore(device, fsd->RenderCompleteSemaphore, allocator);
  fsd->ImageAcquiredSemaphore = fsd->RenderCompleteSemaphore = VK_NULL_HANDLE;
}


// Also destroy old swap chain and in-flight frames data, if any.
void ImplVulkanH_CreateWindowSwapChain(VkPhysicalDevice             physical_device,
                                       VkDevice                     device,
                                       ImGui_ImplVulkanH_Window*    wd,
                                       const VkAllocationCallbacks* allocator,
                                       int                          w,
                                       int                          h,
                                       uint32_t                     min_image_count)
{
  VkSwapchainKHR old_swapchain = wd->Swapchain;
  wd->Swapchain                = VK_NULL_HANDLE;
  NVVK_CHECK(vkDeviceWaitIdle(device));

  // We don't use ImGui_ImplVulkanH_DestroyWindow() because we want to preserve the old swapchain to create the new one.
  // Destroy old Framebuffer
  for(uint32_t i = 0; i < wd->ImageCount; i++)
    ImplVulkanH_DestroyFrame(device, &wd->Frames[i], allocator);
  for(uint32_t i = 0; i < wd->SemaphoreCount; i++)
    ImplVulkanH_DestroyFrameSemaphores(device, &wd->FrameSemaphores[i], allocator);
  IM_FREE(wd->Frames);
  IM_FREE(wd->FrameSemaphores);
  wd->Frames          = nullptr;
  wd->FrameSemaphores = nullptr;
  wd->ImageCount      = 0;
  if(wd->RenderPass)
    vkDestroyRenderPass(device, wd->RenderPass, allocator);
  if(wd->Pipeline)
    vkDestroyPipeline(device, wd->Pipeline, allocator);

  // If min image count was not specified, request different count of images dependent on selected present mode
  if(min_image_count == 0)
    min_image_count = ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(wd->PresentMode);

  // Create Swapchain
  {
    VkSwapchainCreateInfoKHR info = {};
    info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface                  = wd->Surface;
    info.minImageCount            = min_image_count;
    info.imageFormat              = wd->SurfaceFormat.format;
    info.imageColorSpace          = wd->SurfaceFormat.colorSpace;
    info.imageArrayLayers         = 1;
    info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;  // Assume that graphics family == present family
    info.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode              = wd->PresentMode;
    info.clipped                  = VK_TRUE;
    info.oldSwapchain             = old_swapchain;
    VkSurfaceCapabilitiesKHR cap;
    NVVK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, wd->Surface, &cap));
    if(info.minImageCount < cap.minImageCount)
      info.minImageCount = cap.minImageCount;
    else if(cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
      info.minImageCount = cap.maxImageCount;

    if(cap.currentExtent.width == 0xffffffff)
    {
      info.imageExtent.width = wd->Width = w;
      info.imageExtent.height = wd->Height = h;
    }
    else
    {
      info.imageExtent.width = wd->Width = cap.currentExtent.width;
      info.imageExtent.height = wd->Height = cap.currentExtent.height;
    }
    NVVK_CHECK(vkCreateSwapchainKHR(device, &info, allocator, &wd->Swapchain));
    NVVK_CHECK(vkGetSwapchainImagesKHR(device, wd->Swapchain, &wd->ImageCount, nullptr));
    VkImage backbuffers[16] = {};
    IM_ASSERT(wd->ImageCount >= min_image_count);
    IM_ASSERT(wd->ImageCount < IM_ARRAYSIZE(backbuffers));
    NVVK_CHECK(vkGetSwapchainImagesKHR(device, wd->Swapchain, &wd->ImageCount, backbuffers));

    IM_ASSERT(wd->Frames == nullptr && wd->FrameSemaphores == nullptr);
    wd->SemaphoreCount = wd->ImageCount + 1;
    wd->Frames         = (ImGui_ImplVulkanH_Frame*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * wd->ImageCount);
    wd->FrameSemaphores =
        (ImGui_ImplVulkanH_FrameSemaphores*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameSemaphores) * wd->SemaphoreCount);
    memset(wd->Frames, 0, sizeof(wd->Frames[0]) * wd->ImageCount);
    memset(wd->FrameSemaphores, 0, sizeof(wd->FrameSemaphores[0]) * wd->SemaphoreCount);
    for(uint32_t i = 0; i < wd->ImageCount; i++)
      wd->Frames[i].Backbuffer = backbuffers[i];
  }
  if(old_swapchain)
    vkDestroySwapchainKHR(device, old_swapchain, allocator);

  // Create the Render Pass
  if(wd->UseDynamicRendering == false)
  {
    VkAttachmentDescription attachment = {};
    attachment.format                  = wd->SurfaceFormat.format;
    attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = wd->ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment            = 0;
    color_attachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass           = {};
    subpass.pipelineBindPoint              = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount           = 1;
    subpass.pColorAttachments              = &color_attachment;
    VkSubpassDependency dependency         = {};
    dependency.srcSubpass                  = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass                  = 0;
    dependency.srcStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask               = 0;
    dependency.dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info            = {};
    info.sType                             = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount                   = 1;
    info.pAttachments                      = &attachment;
    info.subpassCount                      = 1;
    info.pSubpasses                        = &subpass;
    info.dependencyCount                   = 1;
    info.pDependencies                     = &dependency;
    NVVK_CHECK(vkCreateRenderPass(device, &info, allocator, &wd->RenderPass));

    // We do not create a pipeline by default as this is also used by examples' main.cpp,
    // but secondary viewport in multi-viewport mode may want to create one with:
    //ImGui_ImplVulkan_CreatePipeline(device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline, v->Subpass);
  }

  // Create The Image Views
  {
    VkImageViewCreateInfo info          = {};
    info.sType                          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
    info.format                         = wd->SurfaceFormat.format;
    info.components.r                   = VK_COMPONENT_SWIZZLE_R;
    info.components.g                   = VK_COMPONENT_SWIZZLE_G;
    info.components.b                   = VK_COMPONENT_SWIZZLE_B;
    info.components.a                   = VK_COMPONENT_SWIZZLE_A;
    VkImageSubresourceRange image_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    info.subresourceRange               = image_range;
    for(uint32_t i = 0; i < wd->ImageCount; i++)
    {
      ImGui_ImplVulkanH_Frame* fd = &wd->Frames[i];
      info.image                  = fd->Backbuffer;
      NVVK_CHECK(vkCreateImageView(device, &info, allocator, &fd->BackbufferView));
    }
  }

  // Create Framebuffer
  if(wd->UseDynamicRendering == false)
  {
    VkImageView             attachment[1];
    VkFramebufferCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass              = wd->RenderPass;
    info.attachmentCount         = 1;
    info.pAttachments            = attachment;
    info.width                   = wd->Width;
    info.height                  = wd->Height;
    info.layers                  = 1;
    for(uint32_t i = 0; i < wd->ImageCount; i++)
    {
      ImGui_ImplVulkanH_Frame* fd = &wd->Frames[i];
      attachment[0]               = fd->BackbufferView;
      NVVK_CHECK(vkCreateFramebuffer(device, &info, allocator, &fd->Framebuffer));
    }
  }
}

// Create or resize window
void ImplVulkanH_CreateOrResizeWindow(VkInstance                   instance,
                                      VkPhysicalDevice             physical_device,
                                      VkDevice                     device,
                                      ImGui_ImplVulkanH_Window*    wd,
                                      uint32_t                     queue_family,
                                      const VkAllocationCallbacks* allocator,
                                      int                          width,
                                      int                          height,
                                      uint32_t                     min_image_count)
{
  ImplVulkanH_CreateWindowSwapChain(physical_device, device, wd, allocator, width, height, min_image_count);
  ImplVulkanH_CreateWindowCommandBuffers(physical_device, device, wd, queue_family, allocator);
}
