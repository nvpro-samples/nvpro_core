/*
 * Copyright (c) 2021-2023, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nvvkhl/appbase_vk.hpp"
#include "nvp/perproject_globals.hpp"
// Imgui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui_camera_widget.h"
#include "imgui/imgui_helper.h"


//--------------------------------------------------------------------------------------------------
// Creation order of all elements for the application
// First keep the Vulkan instance, device, ... in class members
// Then create the swapchain, a depth buffer, a default render pass and the
// framebuffers for the swapchain (all sharing the depth image)
// Initialize Imgui and setup callback functions for windows operations (mouse, key, ...)
void nvvkhl::AppBaseVk::create(const AppBaseVkCreateInfo& info)
{
  m_useDynamicRendering = info.useDynamicRendering;
  setup(info.instance, info.device, info.physicalDevice, info.queueIndices[0]);
  createSwapchain(info.surface, info.size.width, info.size.height, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_UNDEFINED, info.useVsync);
  createDepthBuffer();
  createRenderPass();
  createFrameBuffers();
  initGUI();
  setupGlfwCallbacks(info.window);
  ImGui_ImplGlfw_InitForVulkan(info.window, true);
}

//--------------------------------------------------------------------------------------------------
// Setup the low level Vulkan for various operations
//
void nvvkhl::AppBaseVk::setup(const VkInstance& instance, const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex)
{
  m_instance           = instance;
  m_device             = device;
  m_physicalDevice     = physicalDevice;
  m_graphicsQueueIndex = graphicsQueueIndex;
  vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_queue);

  VkCommandPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_cmdPool);

  VkPipelineCacheCreateInfo pipelineCacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
  vkCreatePipelineCache(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache);

  ImGuiH::SetCameraJsonFile(getProjectName());
}

//--------------------------------------------------------------------------------------------------
// To call on exit
//
void nvvkhl::AppBaseVk::destroy()
{
  vkDeviceWaitIdle(m_device);

  if(ImGui::GetCurrentContext() != nullptr)
  {
    // In case multiple ImGUI contexts are used in the same application, the VK side may not own ImGui resources
    if(ImGui::GetIO().BackendRendererUserData)
      ImGui_ImplVulkan_Shutdown();

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  if(!m_useDynamicRendering)
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

  vkDestroyImageView(m_device, m_depthView, nullptr);
  vkDestroyImage(m_device, m_depthImage, nullptr);
  vkFreeMemory(m_device, m_depthMemory, nullptr);
  vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);

  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    vkDestroyFence(m_device, m_waitFences[i], nullptr);

    if(!m_useDynamicRendering)
      vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);

    vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_commandBuffers[i]);
  }
  m_swapChain.deinit();
  vkDestroyDescriptorPool(m_device, m_imguiDescPool, nullptr);
  vkDestroyCommandPool(m_device, m_cmdPool, nullptr);

  if(m_surface)
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Return the surface "screen" for the display
//
VkSurfaceKHR nvvkhl::AppBaseVk::getVkSurface(const VkInstance& instance, GLFWwindow* window)
{
  assert(instance);
  m_window = window;

  VkSurfaceKHR surface{};
  VkResult     err = glfwCreateWindowSurface(instance, window, nullptr, &surface);

  if(err != VK_SUCCESS)
    assert(!"Failed to create a Window surface");

  m_surface = surface;

  return surface;
}

//--------------------------------------------------------------------------------------------------
// Creating the surface for rendering
//
void nvvkhl::AppBaseVk::createSwapchain(const VkSurfaceKHR& surface,
                                        uint32_t            width,
                                        uint32_t            height,
                                        VkFormat            colorFormat /*= VK_FORMAT_B8G8R8A8_UNORM*/,
                                        VkFormat            depthFormat /*= VK_FORMAT_UNDEFINED*/,
                                        bool                vsync /*= false*/)
{
  m_size        = VkExtent2D{width, height};
  m_colorFormat = colorFormat;
  m_depthFormat = depthFormat;
  m_vsync       = vsync;

  // Find the most suitable depth format
  if(m_depthFormat == VK_FORMAT_UNDEFINED)
  {
    auto feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for(const auto& f : {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT})
    {
      VkFormatProperties formatProp{VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
      vkGetPhysicalDeviceFormatProperties(m_physicalDevice, f, &formatProp);
      if((formatProp.optimalTilingFeatures & feature) == feature)
      {
        m_depthFormat = f;
        break;
      }
    }
  }

  m_swapChain.init(m_device, m_physicalDevice, m_queue, m_graphicsQueueIndex, surface, static_cast<VkFormat>(colorFormat));
  m_size        = m_swapChain.update(m_size.width, m_size.height, vsync);
  m_colorFormat = static_cast<VkFormat>(m_swapChain.getFormat());

  // Create Synchronization Primitives
  m_waitFences.resize(m_swapChain.getImageCount());
  for(auto& fence : m_waitFences)
  {
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence);
  }

  // Command buffers store a reference to the frame buffer inside their render pass info
  // so for static usage without having to rebuild them each frame, we use one per frame buffer
  VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocateInfo.commandPool        = m_cmdPool;
  allocateInfo.commandBufferCount = m_swapChain.getImageCount();
  allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  m_commandBuffers.resize(m_swapChain.getImageCount());
  vkAllocateCommandBuffers(m_device, &allocateInfo, m_commandBuffers.data());

  auto cmdBuffer = createTempCmdBuffer();
  m_swapChain.cmdUpdateBarriers(cmdBuffer);
  submitTempCmdBuffer(cmdBuffer);

#ifndef NDEBUG
  for(size_t i = 0; i < m_commandBuffers.size(); i++)
  {
    std::string name = std::string("AppBase") + std::to_string(i);

    VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    nameInfo.objectHandle = (uint64_t)m_commandBuffers[i];
    nameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
    nameInfo.pObjectName  = name.c_str();
    vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
  }
#endif  // !NDEBUG

  // Setup camera
  CameraManip.setWindowSize(m_size.width, m_size.height);
}

//--------------------------------------------------------------------------------------------------
// Create all the framebuffers in which the image will be rendered
// - Swapchain need to be created before calling this
//
void nvvkhl::AppBaseVk::createFrameBuffers()
{
  if(m_useDynamicRendering)
    return;

  // Recreate the frame buffers
  for(auto framebuffer : m_framebuffers)
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);

  // Array of attachment (color, depth)
  std::array<VkImageView, 2> attachments{};

  // Create frame buffers for every swap chain image
  VkFramebufferCreateInfo framebufferCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  framebufferCreateInfo.renderPass      = m_renderPass;
  framebufferCreateInfo.attachmentCount = 2;
  framebufferCreateInfo.width           = m_size.width;
  framebufferCreateInfo.height          = m_size.height;
  framebufferCreateInfo.layers          = 1;
  framebufferCreateInfo.pAttachments    = attachments.data();

  // Create frame buffers for every swap chain image
  m_framebuffers.resize(m_swapChain.getImageCount());
  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    attachments[0] = m_swapChain.getImageView(i);
    attachments[1] = m_depthView;
    vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &m_framebuffers[i]);
  }


#ifndef NDEBUG
  for(size_t i = 0; i < m_framebuffers.size(); i++)
  {
    std::string                   name = std::string("AppBase") + std::to_string(i);
    VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    nameInfo.objectHandle = (uint64_t)m_framebuffers[i];
    nameInfo.objectType   = VK_OBJECT_TYPE_FRAMEBUFFER;
    nameInfo.pObjectName  = name.c_str();
    vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
  }
#endif  // !NDEBUG
}

//--------------------------------------------------------------------------------------------------
// Creating a default render pass, very simple one.
// Other examples will mostly override this one.
//
void nvvkhl::AppBaseVk::createRenderPass()
{
  if(m_useDynamicRendering)
    return;

  if(m_renderPass)
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

  std::array<VkAttachmentDescription, 2> attachments{};
  // Color attachment
  attachments[0].format      = m_colorFormat;
  attachments[0].loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  attachments[0].samples     = VK_SAMPLE_COUNT_1_BIT;

  // Depth attachment
  attachments[1].format        = m_depthFormat;
  attachments[1].loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachments[1].samples       = VK_SAMPLE_COUNT_1_BIT;

  // One color, one depth
  const VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  const VkAttachmentReference depthReference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  std::array<VkSubpassDependency, 1> subpassDependencies{};
  // Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
  subpassDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
  subpassDependencies[0].dstSubpass      = 0;
  subpassDependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkSubpassDescription subpassDescription{};
  subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount    = 1;
  subpassDescription.pColorAttachments       = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;

  VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments    = attachments.data();
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
  renderPassInfo.pDependencies   = subpassDependencies.data();

  vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);

#ifndef NDEBUG
  VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
  nameInfo.objectHandle = (uint64_t)m_renderPass;
  nameInfo.objectType   = VK_OBJECT_TYPE_RENDER_PASS;
  nameInfo.pObjectName  = R"(AppBaseVk)";
  vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
#endif  // !NDEBUG
}

//--------------------------------------------------------------------------------------------------
// Creating an image to be used as depth buffer
//
void nvvkhl::AppBaseVk::createDepthBuffer()
{
  if(m_depthView)
    vkDestroyImageView(m_device, m_depthView, nullptr);

  if(m_depthImage)
    vkDestroyImage(m_device, m_depthImage, nullptr);

  if(m_depthMemory)
    vkFreeMemory(m_device, m_depthMemory, nullptr);

  // Depth information
  const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  VkImageCreateInfo        depthStencilCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  depthStencilCreateInfo.imageType   = VK_IMAGE_TYPE_2D;
  depthStencilCreateInfo.extent      = VkExtent3D{m_size.width, m_size.height, 1};
  depthStencilCreateInfo.format      = m_depthFormat;
  depthStencilCreateInfo.mipLevels   = 1;
  depthStencilCreateInfo.arrayLayers = 1;
  depthStencilCreateInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
  depthStencilCreateInfo.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  // Create the depth image
  vkCreateImage(m_device, &depthStencilCreateInfo, nullptr, &m_depthImage);

#ifndef NDEBUG
  std::string                   name = std::string("AppBaseDepth");
  VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
  nameInfo.objectHandle = (uint64_t)m_depthImage;
  nameInfo.objectType   = VK_OBJECT_TYPE_IMAGE;
  nameInfo.pObjectName  = R"(AppBase)";
  vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
#endif  // !NDEBUG

  // Allocate the memory
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(m_device, m_depthImage, &memReqs);
  VkMemoryAllocateInfo memAllocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memAllocInfo.allocationSize  = memReqs.size;
  memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkAllocateMemory(m_device, &memAllocInfo, nullptr, &m_depthMemory);

  // Bind image and memory
  vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0);

  auto cmdBuffer = createTempCmdBuffer();

  // Put barrier on top, Put barrier inside setup command buffer
  VkImageSubresourceRange subresourceRange{};
  subresourceRange.aspectMask = aspect;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = 1;
  VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageMemoryBarrier.oldLayout             = VK_IMAGE_LAYOUT_UNDEFINED;
  imageMemoryBarrier.newLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  imageMemoryBarrier.image                 = m_depthImage;
  imageMemoryBarrier.subresourceRange      = subresourceRange;
  imageMemoryBarrier.srcAccessMask         = VkAccessFlags();
  imageMemoryBarrier.dstAccessMask         = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  const VkPipelineStageFlags srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  const VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

  vkCmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, VK_FALSE, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
  submitTempCmdBuffer(cmdBuffer);


  // Setting up the view
  VkImageViewCreateInfo depthStencilView{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  depthStencilView.viewType         = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format           = m_depthFormat;
  depthStencilView.subresourceRange = subresourceRange;
  depthStencilView.image            = m_depthImage;
  vkCreateImageView(m_device, &depthStencilView, nullptr, &m_depthView);
}


//--------------------------------------------------------------------------------------------------
// Convenient function to call before rendering.
// - Waits for a framebuffer to be available
// - Update camera matrix if in movement
void nvvkhl::AppBaseVk::prepareFrame()
{
  // Resize protection - should be cached by the glFW callback
  int w, h;
  glfwGetFramebufferSize(m_window, &w, &h);

  if(w != (int)m_size.width || h != (int)m_size.height)
    onFramebufferSize(w, h);

  // Acquire the next image from the swap chain
  if(!m_swapChain.acquire())
    assert(!"This shouldn't happen");

  // Use a fence to wait until the command buffer has finished execution before using it again
  uint32_t imageIndex = m_swapChain.getActiveImageIndex();

  VkResult result{VK_SUCCESS};
  do
  {
    result = vkWaitForFences(m_device, 1, &m_waitFences[imageIndex], VK_TRUE, 1'000'000);
  } while(result == VK_TIMEOUT);
  if(result != VK_SUCCESS)
  {  // This allows Aftermath to do things and later assert below
#ifdef _WIN32
    Sleep(1000);
#else
    usleep(1000);
#endif
  }
  assert(result == VK_SUCCESS);

  // start new frame with updated camera
  updateCamera();
  updateInputs();
}

//--------------------------------------------------------------------------------------------------
// Convenient function to call for submitting the rendering command
// Sending the command buffer of the current frame and add a fence to know when it will be free to use
//
void nvvkhl::AppBaseVk::submitFrame()
{
  uint32_t imageIndex = m_swapChain.getActiveImageIndex();
  vkResetFences(m_device, 1, &m_waitFences[imageIndex]);

  // In case of using NVLINK
  const uint32_t                deviceMask  = m_useNvlink ? 0b0000'0011 : 0b0000'0001;
  const std::array<uint32_t, 2> deviceIndex = {0, 1};

  VkDeviceGroupSubmitInfo deviceGroupSubmitInfo{VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHR};
  deviceGroupSubmitInfo.waitSemaphoreCount            = 1;
  deviceGroupSubmitInfo.commandBufferCount            = 1;
  deviceGroupSubmitInfo.pCommandBufferDeviceMasks     = &deviceMask;
  deviceGroupSubmitInfo.signalSemaphoreCount          = m_useNvlink ? 2 : 1;
  deviceGroupSubmitInfo.pSignalSemaphoreDeviceIndices = deviceIndex.data();
  deviceGroupSubmitInfo.pWaitSemaphoreDeviceIndices   = deviceIndex.data();

  VkSemaphore semaphoreRead  = m_swapChain.getActiveReadSemaphore();
  VkSemaphore semaphoreWrite = m_swapChain.getActiveWrittenSemaphore();

  // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
  const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  // The submit info structure specifies a command buffer queue submission batch
  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.pWaitDstStageMask = &waitStageMask;  // Pointer to the list of pipeline stages that the semaphore waits will occur at
  submitInfo.pWaitSemaphores = &semaphoreRead;  // Semaphore(s) to wait upon before the submitted command buffer starts executing
  submitInfo.waitSemaphoreCount   = 1;                // One wait semaphore
  submitInfo.pSignalSemaphores    = &semaphoreWrite;  // Semaphore(s) to be signaled when command buffers have completed
  submitInfo.signalSemaphoreCount = 1;                // One signal semaphore
  submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];  // Command buffers(s) to execute in this batch (submission)
  submitInfo.commandBufferCount = 1;                           // One command buffer
  submitInfo.pNext              = &deviceGroupSubmitInfo;

  // Submit to the graphics queue passing a wait fence
  vkQueueSubmit(m_queue, 1, &submitInfo, m_waitFences[imageIndex]);

  // Presenting frame
  m_swapChain.present(m_queue);
}

//--------------------------------------------------------------------------------------------------
// When the pipeline is set for using dynamic, this becomes useful
//
void nvvkhl::AppBaseVk::setViewport(const VkCommandBuffer& cmdBuf)
{
  VkViewport viewport{0.0f, 0.0f, static_cast<float>(m_size.width), static_cast<float>(m_size.height), 0.0f, 1.0f};
  vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, {m_size.width, m_size.height}};
  vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
}

//--------------------------------------------------------------------------------------------------
// Window callback when the it is resized
// - Destroy allocated frames, then rebuild them with the new size
// - Call onResize() of the derived class
//
void nvvkhl::AppBaseVk::onFramebufferSize(int w, int h)
{
  if(w == 0 || h == 0)
    return;

  // Update imgui
  if(ImGui::GetCurrentContext() != nullptr)
  {
    auto& imgui_io       = ImGui::GetIO();
    imgui_io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
  }

  // Wait to finish what is currently drawing
  vkDeviceWaitIdle(m_device);
  vkQueueWaitIdle(m_queue);

  // Request new swapchain image size
  m_size         = m_swapChain.update(w, h, m_vsync);
  auto cmdBuffer = createTempCmdBuffer();
  m_swapChain.cmdUpdateBarriers(cmdBuffer);  // Make them presentable
  submitTempCmdBuffer(cmdBuffer);

  if(m_size.width != w || m_size.height != h)
    LOGW("Requested size (%d, %d) is different from created size (%u, %u) ", w, h, m_size.width, m_size.height);

  CameraManip.setWindowSize(m_size.width, m_size.height);
  // Invoking Sample callback
  onResize(m_size.width, m_size.height);  // <-- to implement on derived class
  // Recreating other resources
  createDepthBuffer();
  createFrameBuffers();
}

//--------------------------------------------------------------------------------------------------
// Window callback when the mouse move
// - Handling ImGui and a default camera
//
void nvvkhl::AppBaseVk::onMouseMotion(int x, int y)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
    return;

  if(m_inputs.lmb || m_inputs.rmb || m_inputs.mmb)
    CameraManip.mouseMove(x, y, m_inputs);
}

//--------------------------------------------------------------------------------------------------
// Window callback when a special key gets hit
// - Handling ImGui and a default camera
//
void nvvkhl::AppBaseVk::onKeyboard(int key, int /*scancode*/, int action, int mods)
{
  const bool pressed = action != GLFW_RELEASE;

  if(pressed && key == GLFW_KEY_F11)
    m_show_gui = !m_show_gui;
  else if(pressed && key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(m_window, 1);
}

//--------------------------------------------------------------------------------------------------
// Window callback when a key gets hit
//
void nvvkhl::AppBaseVk::onKeyboardChar(unsigned char key)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
    return;

  // Toggling vsync
  if(key == 'v')
  {
    m_vsync = !m_vsync;
    vkDeviceWaitIdle(m_device);
    vkQueueWaitIdle(m_queue);
    m_swapChain.update(m_size.width, m_size.height, m_vsync);
    auto cmdBuffer = createTempCmdBuffer();
    m_swapChain.cmdUpdateBarriers(cmdBuffer);  // Make them presentable
    submitTempCmdBuffer(cmdBuffer);
    createFrameBuffers();
  }

  if(key == 'h' || key == '?')
    m_showHelp = !m_showHelp;
}


//--------------------------------------------------------------------------------------------------
// Window callback when the mouse button is pressed
// - Handling ImGui and a default camera
//
void nvvkhl::AppBaseVk::onMouseButton(int button, int action, int mods)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
    return;

  double x, y;
  glfwGetCursorPos(m_window, &x, &y);
  CameraManip.setMousePosition(static_cast<int>(x), static_cast<int>(y));
}


//--------------------------------------------------------------------------------------------------
// Window callback when the mouse wheel is modified
// - Handling ImGui and a default camera
//
void nvvkhl::AppBaseVk::onMouseWheel(int delta)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
    return;

  if(delta != 0)
    CameraManip.wheel(delta > 0 ? 1 : -1, m_inputs);
}


//--------------------------------------------------------------------------------------------------
// Called every frame to translate currently pressed keys into camera movement
//
void nvvkhl::AppBaseVk::updateCamera()
{
  // measure one frame at a time
  float factor = ImGui::GetIO().DeltaTime * 1000 * m_sceneRadius;

  m_inputs.lmb   = ImGui::IsMouseDown(ImGuiMouseButton_Left);
  m_inputs.rmb   = ImGui::IsMouseDown(ImGuiMouseButton_Right);
  m_inputs.mmb   = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
  m_inputs.ctrl  = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
  m_inputs.shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
  m_inputs.alt   = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);

  // Allow camera movement only when not editing
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
    return;

  // For all pressed keys - apply the action
  CameraManip.keyMotion(0, 0, nvh::CameraManipulator::NoAction);

  if(!(ImGui::IsKeyDown(ImGuiKey_ModAlt) || ImGui::IsKeyDown(ImGuiKey_ModCtrl) || ImGui::IsKeyDown(ImGuiKey_ModShift)))
  {
    if(ImGui::IsKeyDown(ImGuiKey_W))
      CameraManip.keyMotion(factor, 0, nvh::CameraManipulator::Dolly);

    if(ImGui::IsKeyDown(ImGuiKey_S))
      CameraManip.keyMotion(-factor, 0, nvh::CameraManipulator::Dolly);

    if(ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow))
      CameraManip.keyMotion(factor, 0, nvh::CameraManipulator::Pan);

    if(ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))
      CameraManip.keyMotion(-factor, 0, nvh::CameraManipulator::Pan);

    if(ImGui::IsKeyDown(ImGuiKey_UpArrow))
      CameraManip.keyMotion(0, factor, nvh::CameraManipulator::Pan);

    if(ImGui::IsKeyDown(ImGuiKey_DownArrow))
      CameraManip.keyMotion(0, -factor, nvh::CameraManipulator::Pan);
  }

  // This makes the camera to transition smoothly to the new position
  CameraManip.updateAnim();
}

//--------------------------------------------------------------------------------------------------
// Initialization of the GUI
// - Need to be call after the device creation
//
void nvvkhl::AppBaseVk::initGUI(uint32_t subpassID /*= 0*/)
{
  //assert(m_renderPass && "Render Pass must be set");

  // UI
  ImGui::CreateContext();
  ImGuiIO& io    = ImGui::GetIO();
  io.IniFilename = nullptr;  // Avoiding the INI file
  io.LogFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
  //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

  ImGuiH::setStyle();
  ImGuiH::setFonts();

  std::vector<VkDescriptorPoolSize> poolSize{{VK_DESCRIPTOR_TYPE_SAMPLER, 1}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
  VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets       = 1000;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes    = poolSize.data();
  vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_imguiDescPool);

  // Setup Platform/Renderer back ends
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = m_instance;
  init_info.PhysicalDevice            = m_physicalDevice;
  init_info.Device                    = m_device;
  init_info.QueueFamily               = m_graphicsQueueIndex;
  init_info.Queue                     = m_queue;
  init_info.PipelineCache             = VK_NULL_HANDLE;
  init_info.DescriptorPool            = m_imguiDescPool;
  init_info.Subpass                   = subpassID;
  init_info.MinImageCount             = 2;
  init_info.ImageCount                = static_cast<int>(m_swapChain.getImageCount());
  init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;  // <--- need argument?
  init_info.CheckVkResultFn           = nullptr;
  init_info.Allocator                 = nullptr;

#ifdef VK_KHR_dynamic_rendering
  VkPipelineRenderingCreateInfoKHR rfInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
  if(m_useDynamicRendering)
  {
    rfInfo.colorAttachmentCount    = 1;
    rfInfo.pColorAttachmentFormats = &m_colorFormat;
    rfInfo.depthAttachmentFormat   = m_depthFormat;
    rfInfo.stencilAttachmentFormat = m_depthFormat;
    init_info.rinfo                = &rfInfo;
  }
#endif

  ImGui_ImplVulkan_Init(&init_info, m_renderPass);

  // Upload Fonts
  VkCommandBuffer cmdbuf = createTempCmdBuffer();
  ImGui_ImplVulkan_CreateFontsTexture(cmdbuf);
  submitTempCmdBuffer(cmdbuf);
}

//--------------------------------------------------------------------------------------------------
// Fit the camera to the Bounding box
//
void nvvkhl::AppBaseVk::fitCamera(const nvmath::vec3f& boxMin, const nvmath::vec3f& boxMax, bool instantFit /*= true*/)
{
  CameraManip.fit(boxMin, boxMax, instantFit, false, m_size.width / static_cast<float>(m_size.height));
}

//--------------------------------------------------------------------------------------------------
// Return true if the window is minimized
//
bool nvvkhl::AppBaseVk::isMinimized(bool doSleeping /*= true*/)
{
  int w, h;
  glfwGetWindowSize(m_window, &w, &h);
  bool minimized(w == 0 || h == 0);
  if(minimized && doSleeping)
  {
#ifdef _WIN32
    Sleep(50);
#else
    usleep(50);
#endif
  }
  return minimized;
}

void nvvkhl::AppBaseVk::setupGlfwCallbacks(GLFWwindow* window)
{
  m_window = window;
  glfwSetWindowUserPointer(window, this);
  glfwSetKeyCallback(window, &key_cb);
  glfwSetCharCallback(window, &char_cb);
  glfwSetCursorPosCallback(window, &cursorpos_cb);
  glfwSetMouseButtonCallback(window, &mousebutton_cb);
  glfwSetScrollCallback(window, &scroll_cb);
  glfwSetFramebufferSizeCallback(window, &framebuffersize_cb);
  glfwSetDropCallback(window, &drop_cb);
}

void nvvkhl::AppBaseVk::framebuffersize_cb(GLFWwindow* window, int w, int h)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  app->onFramebufferSize(w, h);
}

void nvvkhl::AppBaseVk::mousebutton_cb(GLFWwindow* window, int button, int action, int mods)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  app->onMouseButton(button, action, mods);
}

void nvvkhl::AppBaseVk::cursorpos_cb(GLFWwindow* window, double x, double y)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  app->onMouseMotion(static_cast<int>(x), static_cast<int>(y));
}

void nvvkhl::AppBaseVk::scroll_cb(GLFWwindow* window, double x, double y)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  app->onMouseWheel(static_cast<int>(y));
}

void nvvkhl::AppBaseVk::key_cb(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  app->onKeyboard(key, scancode, action, mods);
}

void nvvkhl::AppBaseVk::char_cb(GLFWwindow* window, unsigned int key)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  app->onKeyboardChar(key);
}

void nvvkhl::AppBaseVk::drop_cb(GLFWwindow* window, int count, const char** paths)
{
  auto app = static_cast<AppBaseVk*>(glfwGetWindowUserPointer(window));
  int  i;
  for(i = 0; i < count; i++)
    app->onFileDrop(paths[i]);
}

uint32_t nvvkhl::AppBaseVk::getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const
{
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

  for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
  {
    if(((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  }
  LOGE("Unable to find memory type %u\n", static_cast<unsigned int>(properties));
  assert(0);
  return ~0u;
}

// Showing help
void nvvkhl::AppBaseVk::uiDisplayHelp()
{
  if(m_showHelp)
  {
    ImGui::BeginChild("Help", ImVec2(370, 120), true);
    ImGui::Text("%s", CameraManip.getHelp().c_str());
    ImGui::EndChild();
  }
}

VkCommandBuffer nvvkhl::AppBaseVk::createTempCmdBuffer()
{
  // Create an image barrier to change the layout from undefined to DepthStencilAttachmentOptimal
  VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocateInfo.commandBufferCount = 1;
  allocateInfo.commandPool        = m_cmdPool;
  allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  VkCommandBuffer cmdBuffer;
  vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdBuffer);

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmdBuffer, &beginInfo);
  return cmdBuffer;
}

void nvvkhl::AppBaseVk::submitTempCmdBuffer(VkCommandBuffer cmdBuffer)
{
  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmdBuffer;
  vkQueueSubmit(m_queue, 1, &submitInfo, {});
  vkQueueWaitIdle(m_queue);
  vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdBuffer);
}
