/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <filesystem>

#include "application.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "implot.h"
#include "nvh/fileoperations.hpp"
#include "nvvk/error_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvh/timesampler.hpp"
#include "imgui/imgui_helper.h"
#include "Roboto-Regular.h"
#include "imgui/imgui_icon.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#pragma warning(disable : 4996)  // sprintf warning
#include "stb_image_write.h"
#include "imgui/imgui_camera_widget.h"

// Default values
constexpr int32_t  k_imageQuality = 90;
constexpr uint32_t k_maxTextures  = 1000U;
constexpr uint32_t k_maxPool      = 1000U;

// GLFW Callback for file drop
static void dropCb(GLFWwindow* window, int count, const char** paths)
{
  auto* app = static_cast<nvvkhl::Application*>(glfwGetWindowUserPointer(window));
  for(int i = 0; i < count; i++)
  {
    app->onFileDrop(paths[i]);
  }
}

nvvkhl::Application::Application(nvvkhl::ApplicationCreateInfo& info)
{
  init(info);
}

nvvkhl::Application::~Application()
{
  shutdown();
}

void nvvkhl::Application::init(ApplicationCreateInfo& info)
{
  m_instance           = info.instance;
  m_device             = info.device;
  m_physicalDevice     = info.physicalDevice;
  m_queues             = info.queues;
  m_vsyncWanted        = info.vSync;
  m_windowSize         = {info.windowSize.x, info.windowSize.y};
  m_useMenubar         = info.useMenu;
  m_dockSetup          = info.dockSetup;
  m_headless           = info.headless;
  m_headlessFrameCount = info.headlessFrameCount;
  m_viewportSize       = {};  // Will be set by the first viewport size
  if(info.hasUndockableViewport == true)
  {
    info.imguiConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  }

  // Get the executable path and set the log and ini file names
  std::filesystem::path       exePath = nvh::getExecutablePath();
  const std::filesystem::path pathLog = exePath.parent_path() / ("log_" + exePath.stem().string() + ".txt");
  const std::filesystem::path pathIni = exePath.replace_extension(".ini");
  nvprintSetLogFileName(pathLog.string().c_str());
  m_iniFilename = pathIni.string();
  ImGuiH::SetCameraJsonFile(exePath.stem().string());

  // Initialize GLFW and create the window only if not headless
  if(!m_headless)
  {
    initGlfw(info);
  }

  // Used for creating single-time command buffers
  createTransientCommandPool();

  // Create a descriptor pool for creating descriptor set in the application
  createDescriptorPool();

  // Create the swapchain
  if(!m_headless)
  {
    m_swapchain.init(m_physicalDevice, m_device, m_queues[0], m_surface, m_transientCmdPool);
    m_windowSize = m_swapchain.initResources(m_vsyncWanted);  // Update the window size to the actual size of the surface

    // Create what is needed to submit the scene for each frame in-flight
    createFrameSubmission(m_swapchain.getMaxFramesInFlight());
    // Set the resource free queue
    resetFreeQueue(m_swapchain.getMaxFramesInFlight());
  }
  else
  {
    // Headless default size
    if(m_windowSize.width == 0 || m_windowSize.height == 0)
    {
      m_windowSize = {800, 600};
    }
  }

  // Initializing Dear ImGui
  initImGui(info.imguiConfigFlags);
}

void nvvkhl::Application::initGlfw(ApplicationCreateInfo& info)
{
  glfwInit();
  if(m_windowSize.width == 0 || m_windowSize.height == 0)
  {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    m_windowSize.width      = static_cast<int>(static_cast<float>(mode->width) * 0.8F);
    m_windowSize.height     = static_cast<int>(static_cast<float>(mode->height) * 0.8F);
  }

  // Create the GLTF Window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);  // Aware of DPI scaling
  m_windowHandle = glfwCreateWindow(m_windowSize.width, m_windowSize.height, info.name.c_str(), nullptr, nullptr);
  // Set size and position aware of DPI
  glfwSetWindowSize(m_windowHandle, m_windowSize.width, m_windowSize.height);
  glfwSetWindowPos(m_windowHandle, int32_t(m_windowSize.width * 0.1f), int32_t(m_windowSize.height * 0.1f));

  // Create the window surface
  glfwCreateWindowSurface(m_instance, m_windowHandle, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_surface));
  //DBG_VK_NAME(m_surface);

  // Set the Drop callback
  glfwSetWindowUserPointer(m_windowHandle, this);
  glfwSetDropCallback(m_windowHandle, &dropCb);
}

//-----------------------------------------------------------------------
// Shutdown the application
// This will destroy all resources and clean up the application.
void nvvkhl::Application::shutdown()
{
  // Query the size/pos of the window, such that it get persisted
  if(!m_headless)
  {
    glfwGetWindowSize(m_windowHandle, &m_winSize.x, &m_winSize.y);
    glfwGetWindowPos(m_windowHandle, &m_winPos.x, &m_winPos.y);
  }

  // This will call the onDetach of the elements
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onDetach();
  }

  NVVK_CHECK(vkDeviceWaitIdle(m_device));

  // Clean pending
  resetFreeQueue(0);

  // ImGui cleanup
  ImGui_ImplVulkan_Shutdown();
  if(!m_headless)
  {
    ImGui_ImplGlfw_Shutdown();
    m_swapchain.deinit();

    // Frame info
    for(size_t i = 0; i < m_frameData.size(); i++)
    {
      vkFreeCommandBuffers(m_device, m_frameData[i].cmdPool, 1, &m_frameData[i].cmdBuffer);
      vkDestroyCommandPool(m_device, m_frameData[i].cmdPool, nullptr);
    }
    vkDestroySemaphore(m_device, m_frameTimelineSemaphore, nullptr);
  }
  ImGui::DestroyContext();

  if(ImPlot::GetCurrentContext() != nullptr)
  {
    ImPlot::DestroyContext();
  }

  vkDestroyCommandPool(m_device, m_transientCmdPool, nullptr);
  vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

  if(!m_headless)
  {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    // Glfw cleanup
    glfwDestroyWindow(m_windowHandle);
    glfwTerminate();
  }
}

void nvvkhl::Application::addElement(const std::shared_ptr<IAppElement>& layer)
{
  m_elements.emplace_back(layer);
  layer->onAttach(this);
}

void nvvkhl::Application::setVsync(bool v)
{
  m_vsyncWanted = v;
  m_swapchain.requestRebuild();
}

VkCommandBuffer nvvkhl::Application::createTempCmdBuffer() const
{
  return nvvkhl::beginSingleTimeCommands(m_device, m_transientCmdPool);
}

void nvvkhl::Application::submitAndWaitTempCmdBuffer(VkCommandBuffer cmd)
{
  nvvkhl::endSingleTimeCommands(cmd, m_device, m_transientCmdPool, m_queues[0].queue);
}

void nvvkhl::Application::onFileDrop(const char* filename)
{
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onFileDrop(filename);
  }
}

void nvvkhl::Application::close()
{
  glfwSetWindowShouldClose(m_windowHandle, true);
}

//-----------------------------------------------------------------------
// Main loop of the application
// It will run until the window is closed.
// Call all onUIRender() and onRender() for each element.
//
void nvvkhl::Application::run()
{
  if(m_headless)
  {
    headlessRun();
    return;
  }

  ImGui::LoadIniSettingsFromDisk(m_iniFilename.c_str());
  if(isWindowPosValid(m_windowHandle, m_winPos.x, m_winPos.y))
  {
    // Position must be set before size to take into account DPI
    glfwSetWindowPos(m_windowHandle, m_winPos.x, m_winPos.y);
  }
  if(m_winSize != glm::ivec2(0, 0))
  {
    m_windowSize = {uint32_t(m_winSize.x), uint32_t(m_winSize.y)};
    glfwSetWindowSize(m_windowHandle, m_winSize.x, m_winSize.y);
    m_swapchain.requestRebuild();
  }

  // Main rendering loop
  while(!glfwWindowShouldClose(m_windowHandle))
  {
    glfwPollEvents();
    if(glfwGetWindowAttrib(m_windowHandle, GLFW_ICONIFIED) == GLFW_TRUE)
    {
      ImGui_ImplGlfw_Sleep(10);  // Do nothing when minimized
      continue;
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // IMGUI Docking
    // Create a dockspace and dock the viewport and settings window.
    // The central node is named "Viewport", which can be used later with Begin("Viewport")  to render the final image.
    const ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
    ImGuiID dockID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockFlags);
    // Docking layout, must be done only if it doesn't exist
    if(!ImGui::DockBuilderGetNode(dockID)->IsSplitNode() && !ImGui::FindWindowByName("Viewport"))
    {
      ImGui::DockBuilderDockWindow("Viewport", dockID);  // Dock "Viewport" to  central node
      ImGui::DockBuilderGetCentralNode(dockID)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;  // Remove "Tab" from the central node
      if(m_dockSetup)
      {
        // This override allow to create the layout of windows by default.
        m_dockSetup(dockID);
      }
      else
      {
        ImGuiID leftID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.2f, nullptr, &dockID);  // Split the central node
        ImGui::DockBuilderDockWindow("Settings", leftID);  // Dock "Settings" to the left node
      }
    }

    // [optional] Show the menu bar : File, Edit, etc.
    if(m_useMenubar && ImGui::BeginMainMenuBar())
    {
      for(std::shared_ptr<IAppElement>& e : m_elements)
      {
        e->onUIMenu();
      }
      ImGui::EndMainMenuBar();
    }

    // We define the window "Viewport" with no padding an retrieve the rendering area
    VkExtent2D         viewportSize = m_windowSize;
    const ImGuiWindow* viewport     = ImGui::FindWindowByName("Viewport");
    if(viewport)
    {
      viewportSize = {uint32_t(viewport->Size.x), uint32_t(viewport->Size.y)};
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("Viewport");
      ImGui::End();
      ImGui::PopStyleVar();
    }

    // Verify if the viewport has a new size and resize the G-Buffer accordingly.
    if(m_viewportSize.width != viewportSize.width || m_viewportSize.height != viewportSize.height)
    {
      onViewportSizeChange(viewportSize);
    }

    if(m_screenShotRequested && (m_frameRingCurrent == m_screenShotFrame))
    {
      saveScreenShot(m_screenShotFilename, k_imageQuality);
      m_screenShotRequested = false;
    }

    // The main frame rendering
    VkCommandBuffer cmd = beginFrame();
    if(cmd != VK_NULL_HANDLE)
    {
      drawFrame(cmd);
      endFrame(cmd);
      presentFrame();
    }

    // Update and Render additional Platform Windows (floating windows)
    if((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }
    ImGui::EndFrame();
  }
}

//-----------------------------------------------------------------------
// Call this function if the viewport size changes
// This happens when the window is resized, or when the ImGui viewport window is resized.
//
void nvvkhl::Application::onViewportSizeChange(VkExtent2D size)
{
  // Check for DPI scaling and adjust the font size
  float xscale, yscale;
  glfwGetWindowContentScale(m_windowHandle, &xscale, &yscale);
  ImGui::GetIO().FontGlobalScale = xscale;

  m_viewportSize = size;
  // Recreate the G-Buffer to the size of the viewport
  vkQueueWaitIdle(m_queues[0].queue);
  {
    VkCommandBuffer cmd = nvvkhl::beginSingleTimeCommands(m_device, m_transientCmdPool);
    // Call the implementation of the UI rendering
    for(std::shared_ptr<IAppElement>& e : m_elements)
    {
      e->onResize(cmd, m_viewportSize);
    }
    nvvkhl::endSingleTimeCommands(cmd, m_device, m_transientCmdPool, m_queues[0].queue);
  }
}

//-----------------------------------------------------------------------
// Main frame rendering function
// - Acquire the image to render into
// - Call onUIRender() for each element
// - Call onRender() for each element
// - Render the ImGui UI
// - Present the image to the screen
//
void nvvkhl::Application::drawFrame(VkCommandBuffer cmd)
{
  // Reset the extra semaphores and command buffers
  m_waitSemaphores.clear();
  m_signalSemaphores.clear();
  m_commandBuffers.clear();


  // Call UI rendering for each element
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onUIRender();
  }
  ImGui::Render();  // This is creating the data to draw the UI (not on GPU yet)

  // Call render for each element with the command buffer of the frame
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onRender(cmd);
  }

  // Start rendering to the swapchain
  beginDynamicRenderingToSwapchain(cmd);
  {
    // The ImGui draw commands are recorded to the command buffer, which includes the display of our GBuffer image
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  }
  endDynamicRenderingToSwapchain(cmd);
}

//-
// Begin frame is the first step in the rendering process.
// It looks if the swapchain require rebuild, which happens when the window is resized.
// It resets the command pool to reuse the command buffer for recording new rendering commands for the current frame.
// It acquires the image from the swapchain to render into.
// And it returns the command buffer for the frame.
///
VkCommandBuffer nvvkhl::Application::beginFrame()
{
  if(m_swapchain.needRebuilding())
  {
    m_windowSize = m_swapchain.reinitResources(m_vsyncWanted);
  }

  // Get the frame data for the current frame in the ring buffer
  FrameData& frame = m_frameData[m_frameRingCurrent];

  // Wait until GPU has finished processing the frame that was using these resources previously (numFramesInFlight frames ago)
  const uint64_t            waitValue = frame.frameNumber;
  const VkSemaphoreWaitInfo waitInfo  = {
       .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
       .semaphoreCount = 1,
       .pSemaphores    = &m_frameTimelineSemaphore,
       .pValues        = &waitValue,
  };
  vkWaitSemaphores(m_device, &waitInfo, std::numeric_limits<uint64_t>::max());

  // Acquire the image to render into.
  // Beware that this must be happening after the above vkWaitSemaphores.
  // If not, the GPU might still be busy with displaying the frame associated with
  // the to-be acquired image. This in turn means the semaphore we give to vkAcquireNextImageKHR
  // might still have not signaled yet. This causes validation errors as vkAcquireNextImageKHR
  // expects a semaphore with no outstanding GPU work.
  if(!m_swapchain.acquireNextImage(m_device))
    return VK_NULL_HANDLE;

  // Reset the command pool to reuse the command buffer for recording
  // new rendering commands for the current frame.
  NVVK_CHECK(vkResetCommandPool(m_device, frame.cmdPool, 0));
  VkCommandBuffer cmd = frame.cmdBuffer;

  // Begin the command buffer recording for the frame
  const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  NVVK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

  return cmd;
}

//-----------------------------------------------------------------------
// End the frame by submitting the command buffer to the GPU
// Adds binary semaphores to wait for the image to be available and signal when rendering is done.
// Adds the timeline semaphore to signal when the frame is completed.
// Moves to the next frame.
//
void nvvkhl::Application::endFrame(VkCommandBuffer cmd)
{
  // Ends recording of commands for the frame
  NVVK_CHECK(vkEndCommandBuffer(cmd));

  // Prepare to submit the current frame for rendering
  // First add the swapchain semaphore to wait for the image to be available.
  m_waitSemaphores.push_back({
      .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = m_swapchain.getImageAvailableSemaphore(),
      .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // Wait until swapchain image is available before writing to color attachments
  });
  m_signalSemaphores.push_back({
      .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = m_swapchain.getRenderFinishedSemaphore(),
      .stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // Ensures all rendering is complete before presenting
  });


  // Get the frame data for the current frame in the ring buffer
  FrameData& frame = m_frameData[m_frameRingCurrent];

  // Calculate the signal value for when this frame completes
  // Signal value = current frame number + numFramesInFlight
  // Example with 3 frames in flight:
  //   Frame 0 signals value 3 (allowing Frame 3 to start when complete)
  //   Frame 1 signals value 4 (allowing Frame 4 to start when complete)
  const uint64_t signalFrameValue = frame.frameNumber + m_swapchain.getMaxFramesInFlight();
  frame.frameNumber               = signalFrameValue;  // Store for next time this frame buffer is used

  // Add timeline semaphore to signal when GPU completes this frame
  // The color attachment output stage is used since that's when the frame is fully rendered
  m_signalSemaphores.push_back({
      .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = m_frameTimelineSemaphore,
      .value     = signalFrameValue,
      .stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // Ensures all rendering is complete before presenting
  });

  // Adding the command buffer of the frame to the list of command buffers to submit
  // Note: extra command buffers could have been added to the list from other parts of the application (elements)
  m_commandBuffers.push_back({.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cmd});

  // Populate the submit info to synchronize rendering and send the command buffer
  const VkSubmitInfo2 submitInfo{
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .waitSemaphoreInfoCount   = uint32_t(m_waitSemaphores.size()),    //
      .pWaitSemaphoreInfos      = m_waitSemaphores.data(),              // Wait for the image to be available
      .commandBufferInfoCount   = uint32_t(m_commandBuffers.size()),    //
      .pCommandBufferInfos      = m_commandBuffers.data(),              // Command buffer to submit
      .signalSemaphoreInfoCount = uint32_t(m_signalSemaphores.size()),  //
      .pSignalSemaphoreInfos    = m_signalSemaphores.data(),            // Signal when rendering is finished
  };

  // Submit the command buffer to the GPU and signal when it's done
  NVVK_CHECK(vkQueueSubmit2(m_queues[0].queue, 1, &submitInfo, nullptr));
}

//-----------------------------------------------------------------------
// The presentFrame function is the last step in the rendering process.
// It presents the image to the screen and moves to the next frame.
//
void nvvkhl::Application::presentFrame()
{
  // Present the image
  m_swapchain.presentFrame(m_queues[0].queue);

  // Move to the next frame
  m_frameRingCurrent = (m_frameRingCurrent + 1) % m_swapchain.getMaxFramesInFlight();
}


//-----------------------------------------------------------------------
// We are using dynamic rendering, which is a more flexible way to render to the swapchain image.
//
void nvvkhl::Application::beginDynamicRenderingToSwapchain(VkCommandBuffer cmd) const
{
  // Image to render to
  const VkRenderingAttachmentInfoKHR colorAttachment{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
      .imageView   = m_swapchain.getNextImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,   // Clear the image (see clearValue)
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,  // Store the image (keep the image)
      .clearValue  = {{{0.0f, 0.0f, 0.0f, 1.0f}}},
  };

  // Details of the dynamic rendering
  const VkRenderingInfoKHR renderingInfo{
      .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
      .renderArea           = {{0, 0}, m_windowSize},
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &colorAttachment,
  };

  // Transition the swapchain image to the color attachment layout, needed when using dynamic rendering
  nvvk::cmdBarrierImageLayout(cmd, m_swapchain.getNextImage(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  vkCmdBeginRendering(cmd, &renderingInfo);
}

//-----------------------------------------------------------------------
// End of dynamic rendering.
// The image is transitioned back to the present layout, and the rendering is ended.
//
void nvvkhl::Application::endDynamicRenderingToSwapchain(VkCommandBuffer cmd)
{
  vkCmdEndRendering(cmd);

  // Transition the swapchain image back to the present layout
  nvvk::cmdBarrierImageLayout(cmd, m_swapchain.getNextImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

//-----------------------------------------------------------------------
// This is the headless version of the run loop.
// It will render the scene for the number of frames specified in the headlessFrameCount.
// It will call onUIRender() and onRender() for each element.
//
void nvvkhl::Application::headlessRun()
{
  nvh::ScopedTimer st(__FUNCTION__);
  m_viewportSize = m_windowSize;

  VkCommandBuffer cmd = nvvkhl::beginSingleTimeCommands(m_device, m_transientCmdPool);
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onResize(cmd, m_viewportSize);
  }
  nvvkhl::endSingleTimeCommands(cmd, m_device, m_transientCmdPool, m_queues[0].queue);

  ImGuiIO& io      = ImGui::GetIO();
  io.DisplaySize.x = float(m_viewportSize.width);
  io.DisplaySize.y = float(m_viewportSize.height);
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onUIRender();
  }
  ImGui::EndFrame();
  ImGui::Render();

  for(uint32_t frameID = 0; frameID < m_headlessFrameCount; frameID++)
  {
    VkCommandBuffer cmd = nvvkhl::beginSingleTimeCommands(m_device, m_transientCmdPool);
    for(std::shared_ptr<IAppElement>& e : m_elements)
    {
      e->onRender(cmd);
    }
    nvvkhl::endSingleTimeCommands(cmd, m_device, m_transientCmdPool, m_queues[0].queue);
  }

  // Call back the application, such that it can do something with the rendered image
  for(std::shared_ptr<IAppElement>& e : m_elements)
  {
    e->onLastHeadlessFrame();
  }
}

//-----------------------------------------------------------------------
// Create a command pool for short lived operations
// The command pool is used to allocate command buffers.
// In the case of this sample, we only need one command buffer, for temporary execution.
//
void nvvkhl::Application::createTransientCommandPool()
{
  nvvk::DebugUtil debugUtil(m_device);

  const VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,  // Hint that commands will be short-lived
      .queueFamilyIndex = m_queues[0].familyIndex,
  };
  NVVK_CHECK(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_transientCmdPool));
  debugUtil.setObjectName(m_transientCmdPool, "nvvkhl::Application::m_transientCmdPool");
}

//-----------------------------------------------------------------------
// Creates a command pool (long life) and buffer for each frame in flight. Unlike the temporary command pool,
// these pools persist between frames and don't use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT.
// Each frame gets its own command buffer which records all rendering commands for that frame.
//
void nvvkhl::Application::createFrameSubmission(uint32_t numFrames)
{
  nvvk::DebugUtil debugUtil(m_device);

  VkDevice device = m_device;

  m_frameData.resize(numFrames);

  // Initialize timeline semaphore with (numFrames - 1) to allow concurrent frame submission. See details in README.md
  const uint64_t initialValue = (numFrames - 1);

  VkSemaphoreTypeCreateInfo timelineCreateInfo = {
      .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .pNext         = nullptr,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue  = initialValue,
  };

  // Create timeline semaphore for GPU-CPU synchronization
  // This ensures resources aren't overwritten while still in use by the GPU
  const VkSemaphoreCreateInfo semaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timelineCreateInfo};
  NVVK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_frameTimelineSemaphore));
  debugUtil.setObjectName(m_frameTimelineSemaphore, "nvvkhl::Application::m_frameTimelineSemaphore");

  //Create command pools and buffers for each frame
  //Each frame gets its own command pool to allow parallel command recording while previous frames may still be executing on the GPU
  const VkCommandPoolCreateInfo cmdPoolCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = m_queues[0].familyIndex,
  };

  for(uint32_t i = 0; i < numFrames; i++)
  {
    m_frameData[i].frameNumber = i;  // Track frame index for synchronization

    // Separate pools allow independent reset/recording of commands while other frames are still in-flight
    NVVK_CHECK(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &m_frameData[i].cmdPool));
    debugUtil.setObjectName(m_frameData[i].cmdPool, "nvvkhl::AppSwapchain::m_frameData[" + std::to_string(i) + "].cmdPool");

    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_frameData[i].cmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    NVVK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &m_frameData[i].cmdBuffer));
    debugUtil.setObjectName(m_frameData[i].cmdBuffer, "nvvkhl::AppSwapchain::m_frameData[" + std::to_string(i) + "].cmdBuffer");
  }
}

//-----------------------------------------------------------------------
// The Descriptor Pool is used to allocate descriptor sets.
// Currently, only ImGui requires a combined image sampler.
//
void nvvkhl::Application::createDescriptorPool()
{
  nvvk::DebugUtil debugUtil(m_device);

  // Query the physical device properties to determine the maximum number of descriptor sets that can be allocated
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
  uint32_t maxDescriptorSets = std::min(k_maxTextures, deviceProperties.limits.maxDescriptorSetUniformBuffers - 1);
  uint32_t maxTextures       = std::min(k_maxPool, deviceProperties.limits.maxDescriptorSetSampledImages - 1);

  const std::vector<VkDescriptorPoolSize> poolSizes{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures},
  };

  const VkDescriptorPoolCreateInfo poolInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |  //  allows descriptor sets to be updated after they have been bound to a command buffer
               VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,  // individual descriptor sets can be freed from the descriptor pool
      .maxSets       = maxDescriptorSets,  // Allowing to create many sets (ImGui uses this for textures)
      .poolSizeCount = uint32_t(poolSizes.size()),
      .pPoolSizes    = poolSizes.data(),
  };
  NVVK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool));
  debugUtil.setObjectName(m_descriptorPool, "nvvkhl::AppSwapchain::m_descriptorPool");
}

/*-- Initialize ImGui -*/
void nvvkhl::Application::initImGui(ImGuiConfigFlags configFlags)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiH::setStyle(false);

  m_settingsHandler.setHandlerName("Application");
  m_settingsHandler.setSetting("Size", &m_winSize);
  m_settingsHandler.setSetting("Pos", &m_winPos);
  m_settingsHandler.addImGuiHandler();

  ImGuiIO& io    = ImGui::GetIO();
  io.ConfigFlags = configFlags;
  if(m_headless)
  {
    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;  // In headless mode, we don't allow other viewport
  }

  // Set the ini file name
  io.IniFilename = m_iniFilename.c_str();

  // Replace default font with Roboto Regular
  ImFontConfig font_config{};
  font_config.FontDataOwnedByAtlas = false;
  io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&g_Roboto_Regular[0], sizeof(g_Roboto_Regular), 14.0F, &font_config);

  // Add icon font
  ImGuiH::addIconicFont();

  static VkFormat imageFormats = VK_FORMAT_B8G8R8A8_UNORM;  // Must be static for ImGui_ImplVulkan_InitInfo
  if(!m_headless)
  {
    ImGui_ImplGlfw_InitForVulkan(m_windowHandle, true);
    imageFormats = m_swapchain.getImageFormat();
  }

  // ImGui Initialization for Vulkan
  ImGui_ImplVulkan_InitInfo initInfo = {
      .Instance                    = m_instance,
      .PhysicalDevice              = m_physicalDevice,
      .Device                      = m_device,
      .QueueFamily                 = m_queues[0].familyIndex,
      .Queue                       = m_queues[0].queue,
      .DescriptorPool              = m_descriptorPool,
      .MinImageCount               = 2,
      .ImageCount                  = std::max(m_swapchain.getMaxFramesInFlight(), 2U),
      .UseDynamicRendering         = true,
      .PipelineRenderingCreateInfo =  // Dynamic rendering
      {
          .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
          .colorAttachmentCount    = 1,
          .pColorAttachmentFormats = &imageFormats,
      },
  };
  ImGui_ImplVulkan_Init(&initInfo);

  // Implot
  ImPlot::CreateContext();
}

//-----------------------------------------------------------------------
// Convert a tiled image to RGBA8 linear
void nvvkhl::Application::imageToRgba8Linear(VkCommandBuffer  cmd,
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

  nvvk::cmdBarrierImageLayout(cmd, srcImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  nvvk::cmdBarrierImageLayout(cmd, dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Do the actual blit from the swapchain image to our host visible destination image
  // The Blit allow to convert the image from the incoming format to VK_FORMAT_R8G8B8A8_UNORM automatically
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

  nvvk::cmdBarrierImageLayout(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
  nvvk::cmdBarrierImageLayout(cmd, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
}

void nvvkhl::Application::saveImageToFile(VkImage srcImage, VkExtent2D imageSize, const std::string& filename, int quality)
{
  VkDevice         device         = m_device;
  VkPhysicalDevice physicalDevice = m_physicalDevice;
  VkImage          dstImage       = {};
  VkDeviceMemory   dstImageMemory = {};
  VkCommandBuffer  cmd            = createTempCmdBuffer();
  imageToRgba8Linear(cmd, device, physicalDevice, srcImage, imageSize, dstImage, dstImageMemory);
  submitAndWaitTempCmdBuffer(cmd);

  saveImageToFile(device, dstImage, dstImageMemory, imageSize, filename, quality);

  // Clean up resources
  vkUnmapMemory(device, dstImageMemory);
  vkFreeMemory(device, dstImageMemory, nullptr);
  vkDestroyImage(device, dstImage, nullptr);
}

// Save an image to a file
// The image should be in Rgba8 (linear) format
void nvvkhl::Application::saveImageToFile(VkDevice           device,
                                          VkImage            linearImage,
                                          VkDeviceMemory     imageMemory,
                                          VkExtent2D         imageSize,
                                          const std::string& filename,
                                          int                quality)
{
  // Get layout of the image (including offset and row pitch)
  VkImageSubresource  subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
  VkSubresourceLayout subResourceLayout;
  vkGetImageSubresourceLayout(device, linearImage, &subResource, &subResourceLayout);

  // Map image memory so we can start copying from it
  const char* data = nullptr;
  vkMapMemory(device, imageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
  data += subResourceLayout.offset;

  // Copy the data and adjust for the row pitch
  std::vector<uint8_t> pixels(imageSize.width * imageSize.height * 4);
  for(uint32_t y = 0; y < imageSize.height; y++)
  {
    memcpy(pixels.data() + y * imageSize.width * 4, data, static_cast<size_t>(imageSize.width) * 4);
    data += subResourceLayout.rowPitch;
  }

  std::filesystem::path path      = filename;
  std::string           extension = path.extension().string();

  // Check the extension and perform actions accordingly
  if(extension == ".png")
  {
    stbi_write_png(filename.c_str(), imageSize.width, imageSize.height, 4, pixels.data(), imageSize.width * 4);
  }
  else if(extension == ".jpg" || extension == ".jpeg")
  {
    stbi_write_jpg(filename.c_str(), imageSize.width, imageSize.height, 4, pixels.data(), quality);
  }
  else if(extension == ".bmp")
  {
    stbi_write_bmp(filename.c_str(), imageSize.width, imageSize.height, 4, pixels.data());
  }
  else
  {
    LOGW("Screenshot: unknown file extension, saving as PNG\n");
    path += ".png";
    stbi_write_png(path.string().c_str(), imageSize.width, imageSize.height, 4, pixels.data(), imageSize.width * 4);
  }

  LOGI("Image saved to %s\n", filename.c_str());
}

// Record that a screenshot is requested, and will be saved at the end of the frame (saveScreenShot)
void nvvkhl::Application::screenShot(const std::string& filename, int quality)
{
  m_screenShotRequested = true;
  m_screenShotFilename  = filename;
  // Making sure the screenshot is taken after the swapchain loop (remove the menu after click)
  m_screenShotFrame = (m_frameRingCurrent - 1 + m_swapchain.getMaxFramesInFlight()) % m_swapchain.getMaxFramesInFlight();
}

// Save the current swapchain image to a file
void nvvkhl::Application::saveScreenShot(const std::string& filename, int quality)
{
  VkExtent2D     size     = m_windowSize;
  VkImage        srcImage = m_swapchain.getNextImage();
  VkImage        dstImage;
  VkDeviceMemory dstImageMemory;

  vkDeviceWaitIdle(m_device);
  VkCommandBuffer cmd = createTempCmdBuffer();
  nvvk::cmdBarrierImageLayout(cmd, srcImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL);
  imageToRgba8Linear(cmd, m_device, m_physicalDevice, srcImage, size, dstImage, dstImageMemory);
  nvvk::cmdBarrierImageLayout(cmd, srcImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  submitAndWaitTempCmdBuffer(cmd);

  saveImageToFile(m_device, dstImage, dstImageMemory, size, filename, quality);

  // Clean up resources
  vkUnmapMemory(m_device, dstImageMemory);
  vkFreeMemory(m_device, dstImageMemory, nullptr);
  vkDestroyImage(m_device, dstImage, nullptr);
}


//////////////////////////////////////////////////////////////////////////
//
//
// Set the viewport using with the size of the "Viewport" window
void nvvkhl::Application::setViewport(const VkCommandBuffer& cmd)
{
  VkViewport viewport{0.0F, 0.0F, static_cast<float>(m_viewportSize.width), static_cast<float>(m_viewportSize.height),
                      0.0F, 1.0F};
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, {m_viewportSize.width, m_viewportSize.height}};
  vkCmdSetScissor(cmd, 0, 1, &scissor);
}


void nvvkhl::Application::submitResourceFree(std::function<void()>&& func)
{
  if(m_frameRingCurrent < m_resourceFreeQueue.size())
  {
    m_resourceFreeQueue[m_frameRingCurrent].emplace_back(func);
  }
  else
  {
    func();
  }
}

void nvvkhl::Application::resetFreeQueue(uint32_t size)
{
  vkDeviceWaitIdle(m_device);

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


//-----------------------------------------------------------------------
// Helpers

// Check if window position is within visible monitor bounds
bool nvvkhl::Application::isWindowPosValid(GLFWwindow* window, int posX, int posY)
{
  int           monitorCount;
  GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

  // For each connected monitor
  for(int i = 0; i < monitorCount; i++)
  {
    GLFWmonitor*       monitor = monitors[i];
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

    int monX, monY;
    glfwGetMonitorPos(monitor, &monX, &monY);

    // Check if window position is within this monitor's bounds
    // Add some margin to account for window decorations
    if(posX >= monX && posX < monX + mode->width && posY >= monY && posY < monY + mode->height)
    {
      return true;
    }
  }

  return false;
}