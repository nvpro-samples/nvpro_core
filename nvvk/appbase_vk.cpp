/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#include "nvvk/appbase_vk.hpp"


#ifndef PROJECT_NAME
#define PROJECT_NAME "AppBaseVk"
#endif

//--------------------------------------------------------------------------------------------------
// Setup the low level Vulkan for various operations
//
void nvvk::AppBaseVk::setup(const VkInstance& instance, const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex)
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

  ImGuiH::SetCameraJsonFile(PROJECT_NAME);
}

//--------------------------------------------------------------------------------------------------
// To call on exit
//
void nvvk::AppBaseVk::destroy()
{
  vkDeviceWaitIdle(m_device);

  if(ImGui::GetCurrentContext() != nullptr)
  {
    //ImGui::ShutdownVK();
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
  }

  vkDestroyRenderPass(m_device, m_renderPass, nullptr);
  vkDestroyImageView(m_device, m_depthView, nullptr);
  vkDestroyImage(m_device, m_depthImage, nullptr);
  vkFreeMemory(m_device, m_depthMemory, nullptr);
  vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);

  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    vkDestroyFence(m_device, m_waitFences[i], nullptr);
    vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
    vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_commandBuffers[i]);
  }
  m_swapChain.deinit();
  vkDestroyDescriptorPool(m_device, m_imguiDescPool, nullptr);
  vkDestroyCommandPool(m_device, m_cmdPool, nullptr);

  if(m_surface)
  {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  }
}

//--------------------------------------------------------------------------------------------------
// Return the surface "screen" for the display
//
VkSurfaceKHR nvvk::AppBaseVk::getVkSurface(const VkInstance& instance, GLFWwindow* window)
{
  assert(instance);
  m_window = window;

  VkSurfaceKHR surface{};
  VkResult     err = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if(err != VK_SUCCESS)
  {
    assert(!"Failed to create a Window surface");
  }
  m_surface = surface;

  return surface;
}

//--------------------------------------------------------------------------------------------------
// Creating the surface for rendering
//
void nvvk::AppBaseVk::createSwapchain(const VkSurfaceKHR& surface,
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


#ifdef _DEBUG
  for(size_t i = 0; i < m_commandBuffers.size(); i++)
  {
    std::string name = std::string("AppBase") + std::to_string(i);

    VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    nameInfo.objectHandle = (uint64_t)m_commandBuffers[i];
    nameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
    nameInfo.pObjectName  = name.c_str();
    vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
  }
#endif  // _DEBUG

  // Setup camera
  CameraManip.setWindowSize(m_size.width, m_size.height);
}

//--------------------------------------------------------------------------------------------------
// Create all the framebuffers in which the image will be rendered
// - Swapchain need to be created before calling this
//
void nvvk::AppBaseVk::createFrameBuffers()
{
  // Recreate the frame buffers
  for(auto framebuffer : m_framebuffers)
  {
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
  }

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


#ifdef _DEBUG
  for(size_t i = 0; i < m_framebuffers.size(); i++)
  {
    std::string                   name = std::string("AppBase") + std::to_string(i);
    VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    nameInfo.objectHandle = (uint64_t)m_framebuffers[i];
    nameInfo.objectType   = VK_OBJECT_TYPE_FRAMEBUFFER;
    nameInfo.pObjectName  = name.c_str();
    vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
  }
#endif  // _DEBUG
}

//--------------------------------------------------------------------------------------------------
// Creating a default render pass, very simple one.
// Other examples will mostly override this one.
//
void nvvk::AppBaseVk::createRenderPass()
{
  if(m_renderPass)
  {
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
  }

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

#ifdef _DEBUG
  VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
  nameInfo.objectHandle = (uint64_t)m_renderPass;
  nameInfo.objectType   = VK_OBJECT_TYPE_RENDER_PASS;
  nameInfo.pObjectName  = R"(AppBaseVk)";
  vkSetDebugUtilsObjectNameEXT(m_device, &nameInfo);
#endif  // _DEBUG
}

//--------------------------------------------------------------------------------------------------
// Creating an image to be used as depth buffer
//
void nvvk::AppBaseVk::createDepthBuffer()
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

  // Allocate the memory
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(m_device, m_depthImage, &memReqs);
  VkMemoryAllocateInfo memAllocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memAllocInfo.allocationSize  = memReqs.size;
  memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkAllocateMemory(m_device, &memAllocInfo, nullptr, &m_depthMemory);

  // Bind image and memory
  vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0);

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
  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmdBuffer;
  vkQueueSubmit(m_queue, 1, &submitInfo, {});
  vkQueueWaitIdle(m_queue);
  vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdBuffer);

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
// Waits for a framebuffer to be available
//
void nvvk::AppBaseVk::prepareFrame()
{
  // Resize protection - should be cached by the glFW callback
  int w, h;
  glfwGetFramebufferSize(m_window, &w, &h);
  if(w != (int)m_size.width || h != (int)m_size.height)
  {
    onFramebufferSize(w, h);
  }

  // Acquire the next image from the swap chain
  if(!m_swapChain.acquire())
  {
    assert(!"This shouldn't happen");
  }

  // Use a fence to wait until the command buffer has finished execution before using it again
  uint32_t imageIndex = m_swapChain.getActiveImageIndex();

  while(vkWaitForFences(m_device, 1, &m_waitFences[imageIndex], VK_TRUE, 10000) == VK_TIMEOUT)
  {
  }
}

//--------------------------------------------------------------------------------------------------
// Convenient function to call for submitting the rendering command
// Sending the command buffer of the current frame and add a fence to know when it will be free to use
//
void nvvk::AppBaseVk::submitFrame()
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
void nvvk::AppBaseVk::setViewport(const VkCommandBuffer& cmdBuf)
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
void nvvk::AppBaseVk::onFramebufferSize(int w, int h)
{
  if(w == 0 || h == 0)
  {
    return;
  }

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
  m_size = m_swapChain.update(m_size.width, m_size.height, m_vsync);

  if(m_size.width != w || m_size.height != h)
  {
    LOGW("Requested size (%d, %d) is different from created size (%u, %u) ", w, h, m_size.width, m_size.height);
  }

  CameraManip.setWindowSize(m_size.width, m_size.height);
  // Invoking Sample callback
  onResize(m_size.width, m_size.height); // <-- to implement on derived class
  // Recreating other resources
  createDepthBuffer();
  createFrameBuffers();
}

//--------------------------------------------------------------------------------------------------
// Window callback when the mouse move
// - Handling ImGui and a default camera
//
void nvvk::AppBaseVk::onMouseMotion(int x, int y)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  if(m_inputs.lmb || m_inputs.rmb || m_inputs.mmb)
  {
    CameraManip.mouseMove(x, y, m_inputs);
  }
}


//--------------------------------------------------------------------------------------------------
// Window callback when a special key gets hit
// - Handling ImGui and a default camera
//
void nvvk::AppBaseVk::onKeyboard(int key, int scancode, int action, int mods)
{
  const bool capture = ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
  const bool pressed = action != GLFW_RELEASE;

  // Keeping track of the modifiers
  m_inputs.ctrl  = mods & GLFW_MOD_CONTROL;
  m_inputs.shift = mods & GLFW_MOD_SHIFT;
  m_inputs.alt   = mods & GLFW_MOD_ALT;

  // Remember all keys that are pressed for animating the camera when
  // many keys are pressed and stop when all keys are released.
  if(pressed)
  {
    m_keys.insert(key);
  }
  else
  {
    m_keys.erase(key);
  }

  // For all pressed keys - apply the action
  CameraManip.keyMotion(0, 0, nvh::CameraManipulator::NoAction);
  for(auto key : m_keys)
  {
    switch(key)
    {
      case GLFW_KEY_F10:
        m_show_gui = !m_show_gui;
        break;
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(m_window, 1);
        break;
      default:
        break;
    }

    // Allow camera movement only when not editing
    if(!capture)
    {
      switch(key)
      {
        case GLFW_KEY_W:
          CameraManip.keyMotion(1.f, 0, nvh::CameraManipulator::Dolly);
          break;
        case GLFW_KEY_S:
          CameraManip.keyMotion(-1.f, 0, nvh::CameraManipulator::Dolly);
          break;
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
          CameraManip.keyMotion(-1.f, 0, nvh::CameraManipulator::Pan);
          break;
        case GLFW_KEY_UP:
          CameraManip.keyMotion(0, 1, nvh::CameraManipulator::Pan);
          break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
          CameraManip.keyMotion(1.f, 0, nvh::CameraManipulator::Pan);
          break;
        case GLFW_KEY_DOWN:
          CameraManip.keyMotion(0, -1, nvh::CameraManipulator::Pan);
          break;
        default:
          break;
      }
    }
  }
}


//--------------------------------------------------------------------------------------------------
// Window callback when a key gets hit
//
void nvvk::AppBaseVk::onKeyboardChar(unsigned char key)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
  {
    return;
  }

  // Toggling vsync
  if(key == 'v')
  {
    m_vsync = !m_vsync;
    vkDeviceWaitIdle(m_device);
    vkQueueWaitIdle(m_queue);
    m_swapChain.update(m_size.width, m_size.height, m_vsync);
    createFrameBuffers();
  }

  if(key == 'h' || key == '?')
    m_showHelp = !m_showHelp;
}


//--------------------------------------------------------------------------------------------------
// Window callback when the mouse button is pressed
// - Handling ImGui and a default camera
//
void nvvk::AppBaseVk::onMouseButton(int button, int action, int mods)
{
  (void)mods;

  double x, y;
  glfwGetCursorPos(m_window, &x, &y);
  CameraManip.setMousePosition(static_cast<int>(x), static_cast<int>(y));

  m_inputs.lmb = (button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS);
  m_inputs.mmb = (button == GLFW_MOUSE_BUTTON_MIDDLE) && (action == GLFW_PRESS);
  m_inputs.rmb = (button == GLFW_MOUSE_BUTTON_RIGHT) && (action == GLFW_PRESS);
}


//--------------------------------------------------------------------------------------------------
// Window callback when the mouse wheel is modified
// - Handling ImGui and a default camera
//
void nvvk::AppBaseVk::onMouseWheel(int delta)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  CameraManip.wheel(delta > 0 ? 1 : -1, m_inputs);
}

//--------------------------------------------------------------------------------------------------
// Initialization of the GUI
// - Need to be call after the device creation
//
void nvvk::AppBaseVk::initGUI(uint32_t subpassID /*= 0*/)
{
  assert(m_renderPass && "Render Pass must be set");

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
  VkDescriptorPoolCreateInfo        poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets       = 2;
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
  init_info.ImageCount                = static_cast<int>(m_framebuffers.size());
  init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;  // <--- need argument?
  init_info.CheckVkResultFn           = nullptr;
  init_info.Allocator                 = nullptr;
  ImGui_ImplVulkan_Init(&init_info, m_renderPass);


  // Upload Fonts
  VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocateInfo.commandBufferCount = 1;
  allocateInfo.commandPool        = m_cmdPool;
  allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  VkCommandBuffer cmdbuf;
  vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdbuf);

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmdbuf, &beginInfo);

  ImGui_ImplVulkan_CreateFontsTexture(cmdbuf);

  vkEndCommandBuffer(cmdbuf);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmdbuf;
  vkQueueSubmit(m_queue, 1, &submitInfo, {});
  vkQueueWaitIdle(m_queue);
  vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdbuf);
}

//--------------------------------------------------------------------------------------------------
// Fit the camera to the Bounding box
//
void nvvk::AppBaseVk::fitCamera(const nvmath::vec3f& boxMin, const nvmath::vec3f& boxMax, bool instantFit /*= true*/)
{
  CameraManip.fit(boxMin, boxMax, instantFit, false, m_size.width / static_cast<float>(m_size.height));
}

//--------------------------------------------------------------------------------------------------
// Return true if the window is minimized
//
bool nvvk::AppBaseVk::isMinimized(bool doSleeping /*= true*/)
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

void nvvk::AppBaseVk::setupGlfwCallbacks(GLFWwindow* window)
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

uint32_t nvvk::AppBaseVk::getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties) const
{
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

  for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
  {
    if(((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }
  std::string err = "Unable to find memory type " + std::to_string(properties);
  LOGE(err.c_str());
  assert(0);
  return ~0u;
}

void nvvk::AppBaseVk::uiDisplayHelp()
{
  if(m_showHelp)
  {
    ImGui::BeginChild("Help", ImVec2(370, 120), true);
    ImGui::Text("%s", CameraManip.getHelp().c_str());
    ImGui::EndChild();
  }
}
