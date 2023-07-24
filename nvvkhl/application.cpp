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


/*
    The Application is basically a small modification of the ImGui example for Vulkan.
    Because we support multiple viewports, duplicating the code would be not necessary 
    and the code is very well explained. 

    Worth notice

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

#include "application.hpp"

#include "backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui_camera_widget.h"
#include "imgui/imgui_helper.h"
#include "nvp/nvpsystem.hpp"
#include "nvp/perproject_globals.hpp"
#include "nvvk/context_vk.hpp"
#include "nvvk/error_vk.hpp"


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

// Static
uint32_t                                        nvvkhl::Application::m_currentFrameIndex{0};
std::vector<std::vector<std::function<void()>>> nvvkhl::Application::m_resourceFreeQueue;

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

  m_clearColor = info.clearColor;
  m_dockSetup  = info.dockSetup;

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

  m_resourceFreeQueue.resize(wd->ImageCount);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  m_iniFilename  = path_ini.string();
  io.IniFilename = m_iniFilename.c_str();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

  // Setup Dear ImGui style
  ImGuiH::setStyle();

  // Load default font
  const float  high_dpi_scale = ImGuiH::getDPIScale();
  ImFontConfig font_config;
  font_config.FontDataOwnedByAtlas = false;
  io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&g_Roboto_Regular[0], sizeof(g_Roboto_Regular),
                                                  14.0F * high_dpi_scale, &font_config);

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
  init_info.Subpass                   = 0;
  init_info.MinImageCount             = m_minImageCount;
  init_info.ImageCount                = wd->ImageCount;
  init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator                 = m_allocator;
  init_info.CheckVkResultFn           = checkVkResult;
  ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);


  // Upload Fonts
  VkCommandBuffer cmd = createTempCmdBuffer();
  ImGui_ImplVulkan_CreateFontsTexture(cmd);
  submitAndWaitTempCmdBuffer(cmd);
  ImGui_ImplVulkan_DestroyFontUploadObjects();


  // Read camera setting
  ImGuiH::SetCameraJsonFile(getProjectName());
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
  for(auto& free_queue : m_resourceFreeQueue)
  {
    for(auto& func : free_queue)
    {
      func();
    }
    free_queue.clear();
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
  ImGui_ImplVulkanH_CreateOrResizeWindow(m_context->m_instance, m_context->m_physicalDevice, m_context->m_device, wd,
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
        ImGui_ImplVulkanH_CreateOrResizeWindow(m_context->m_instance, m_context->m_physicalDevice, m_context->m_device,
                                               m_mainWindowData.get(), m_context->m_queueGCT.familyIndex, m_allocator,
                                               width, height, m_minImageCount);
        m_resourceFreeQueue.resize(wd->ImageCount);
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

      ImGui::Begin("DockSpace NVVKHL");

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
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
      ImGui::PushStyleColor(ImGuiCol_WindowBg, m_clearColor);
      ImGui::Begin("Viewport");
      ImVec2 viewport_size         = ImGui::GetContentRegionAvail();
      bool   viewport_size_changed = (static_cast<uint32_t>(viewport_size.x) != m_viewportSize.width
                                    || static_cast<uint32_t>(viewport_size.y) != m_viewportSize.height);
      ImGui::End();
      ImGui::PopStyleVar();
      ImGui::PopStyleColor();

      if(viewport_size_changed)
      {
        m_viewportSize.width  = static_cast<uint32_t>(viewport_size.x);
        m_viewportSize.height = static_cast<uint32_t>(viewport_size.y);
        for(auto& e : m_elements)
        {
          e->onResize(m_viewportSize.width, m_viewportSize.height);
        }
      }

      // Call the implementation of the UI rendering
      for(auto& e : m_elements)
      {
        e->onUIRender();
      }

      ImGui::End();
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

    // Update and Render additional Platform Windows
    if((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
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

  dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
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

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkCommandBufferSubmitInfo cmdBufInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdBufInfo.commandBuffer = fd->CommandBuffer;

    VkSemaphoreSubmitInfo signalSemaphore{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signalSemaphore.semaphore = render_complete_semaphore;
    signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_signalSemaphores.emplace_back(signalSemaphore);

    VkSemaphoreSubmitInfo waitSemaphore{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    waitSemaphore.semaphore = image_acquired_semaphore;
    waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_waitSemaphores.emplace_back(waitSemaphore);

    VkSubmitInfo2 submits{VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
    submits.commandBufferInfoCount   = 1;
    submits.pCommandBufferInfos      = &cmdBufInfo;
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
  wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;  // Now we can use the next set of semaphores
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
