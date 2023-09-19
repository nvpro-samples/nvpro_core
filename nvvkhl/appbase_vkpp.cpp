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

#include "nvvkhl/appbase_vkpp.hpp"

#include "imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui_helper.h"
#include "imgui/imgui_camera_widget.h"
#include "nvp/perproject_globals.hpp"

#ifdef LINUX
#include <unistd.h>
#endif

#include "GLFW/glfw3.h"

#include <cmath>

void nvvkhl::AppBase::onResize(int, int) {}

void nvvkhl::AppBase::setup(const vk::Instance& instance, const vk::Device& device, const vk::PhysicalDevice& physicalDevice, uint32_t graphicsQueueIndex)
{
  // Initialize function pointers
  vk::DynamicLoader         dl;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
      dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(instance, device);

  m_instance           = instance;
  m_device             = device;
  m_physicalDevice     = physicalDevice;
  m_graphicsQueueIndex = graphicsQueueIndex;
  m_queue              = m_device.getQueue(m_graphicsQueueIndex, 0);
  m_cmdPool = m_device.createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueIndex});
  m_pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

  ImGuiH::SetCameraJsonFile(getProjectName());
}

void nvvkhl::AppBase::destroy()
{
  m_device.waitIdle();

  if(ImGui::GetCurrentContext() != nullptr)
  {
    //ImGui::ShutdownVK();
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
  }

  m_device.destroy(m_renderPass);
  m_device.destroy(m_depthView);
  m_device.destroy(m_depthImage);
  m_device.freeMemory(m_depthMemory);
  m_device.destroy(m_pipelineCache);

  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    m_device.destroy(m_waitFences[i]);
    m_device.destroy(m_framebuffers[i]);
    m_device.freeCommandBuffers(m_cmdPool, m_commandBuffers[i]);
  }
  m_swapChain.deinit();
  m_device.destroy(m_imguiDescPool);

  m_device.destroy(m_cmdPool);
  if(m_surface)
    m_instance.destroySurfaceKHR(m_surface);
}

VkSurfaceKHR nvvkhl::AppBase::getVkSurface(const vk::Instance& instance, GLFWwindow* window)
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

void nvvkhl::AppBase::createSwapchain(const vk::SurfaceKHR& surface, uint32_t width, uint32_t height, vk::Format colorFormat, vk::Format depthFormat, bool vsync)
{
  m_size        = vk::Extent2D(width, height);
  m_colorFormat = colorFormat;
  m_depthFormat = depthFormat;
  m_vsync       = vsync;

  // Find the most suitable depth format
  if(m_depthFormat == vk::Format::eUndefined)
  {
    auto feature = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
    for(const auto& f : {vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint})
    {
      if((m_physicalDevice.getFormatProperties(f).optimalTilingFeatures & feature) == feature)
      {
        m_depthFormat = f;
        break;
      }
    }
  }

  m_swapChain.init(m_device, m_physicalDevice, m_queue, m_graphicsQueueIndex, surface, static_cast<VkFormat>(colorFormat));
  m_size        = m_swapChain.update(m_size.width, m_size.height, vsync);
  m_colorFormat = static_cast<vk::Format>(m_swapChain.getFormat());

  // Create Synchronization Primitives
  m_waitFences.resize(m_swapChain.getImageCount());
  for(auto& fence : m_waitFences)
  {
    fence = m_device.createFence({vk::FenceCreateFlagBits::eSignaled});
  }

  // Command buffers store a reference to the frame buffer inside their render pass info
  // so for static usage without having to rebuild them each frame, we use one per frame buffer
  m_commandBuffers =
      m_device.allocateCommandBuffers({m_cmdPool, vk::CommandBufferLevel::ePrimary, m_swapChain.getImageCount()});

#ifndef NDEBUG
  for(size_t i = 0; i < m_commandBuffers.size(); i++)
  {
    std::string name = std::string("AppBase") + std::to_string(i);
    m_device.setDebugUtilsObjectNameEXT(
        {vk::ObjectType::eCommandBuffer, reinterpret_cast<const uint64_t&>(m_commandBuffers[i]), name.c_str()});
  }
#endif  // !NDEBUG

  // Setup camera
  CameraManip.setWindowSize(m_size.width, m_size.height);
}

void nvvkhl::AppBase::createFrameBuffers()
{
  // Recreate the frame buffers
  for(auto framebuffer : m_framebuffers)
  {
    m_device.destroy(framebuffer);
  }

  // Array of attachment (color, depth)
  std::array<vk::ImageView, 2> attachments;

  // Create frame buffers for every swap chain image
  vk::FramebufferCreateInfo framebufferCreateInfo;
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
    attachments[0]    = m_swapChain.getImageView(i);
    attachments[1]    = m_depthView;
    m_framebuffers[i] = m_device.createFramebuffer(framebufferCreateInfo);
  }


#ifndef NDEBUG
  for(size_t i = 0; i < m_framebuffers.size(); i++)
  {
    std::string name = std::string("AppBase") + std::to_string(i);
    m_device.setDebugUtilsObjectNameEXT(
        {vk::ObjectType::eFramebuffer, reinterpret_cast<const uint64_t&>(m_framebuffers[i]), name.c_str()});
  }
#endif  // !NDEBUG
}

void nvvkhl::AppBase::createRenderPass()
{
  if(m_renderPass)
  {
    m_device.destroy(m_renderPass);
  }

  std::array<vk::AttachmentDescription, 2> attachments;
  // Color attachment
  attachments[0].setFormat(m_colorFormat);
  attachments[0].setLoadOp(vk::AttachmentLoadOp::eClear);
  attachments[0].setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

  // Depth attachment
  attachments[1].setFormat(m_depthFormat);
  attachments[1].setLoadOp(vk::AttachmentLoadOp::eClear);
  attachments[1].setStencilLoadOp(vk::AttachmentLoadOp::eClear);
  attachments[1].setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  // One color, one depth
  const vk::AttachmentReference colorReference{0, vk::ImageLayout::eColorAttachmentOptimal};
  const vk::AttachmentReference depthReference{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

  std::array<vk::SubpassDependency, 1> subpassDependencies;
  // Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
  subpassDependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL);
  subpassDependencies[0].setDstSubpass(0);
  subpassDependencies[0].setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe);
  subpassDependencies[0].setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
  subpassDependencies[0].setSrcAccessMask(vk::AccessFlagBits::eMemoryRead);
  subpassDependencies[0].setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
  subpassDependencies[0].setDependencyFlags(vk::DependencyFlagBits::eByRegion);

  vk::SubpassDescription subpassDescription;
  subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
  subpassDescription.setColorAttachmentCount(1);
  subpassDescription.setPColorAttachments(&colorReference);
  subpassDescription.setPDepthStencilAttachment(&depthReference);

  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
  renderPassInfo.setPAttachments(attachments.data());
  renderPassInfo.setSubpassCount(1);
  renderPassInfo.setPSubpasses(&subpassDescription);
  renderPassInfo.setDependencyCount(static_cast<uint32_t>(subpassDependencies.size()));
  renderPassInfo.setPDependencies(subpassDependencies.data());

  m_renderPass = m_device.createRenderPass(renderPassInfo);

#ifndef NDEBUG
  m_device.setDebugUtilsObjectNameEXT(
      {vk::ObjectType::eRenderPass, reinterpret_cast<const uint64_t&>(m_renderPass), "AppBase"});
#endif  // !NDEBUG
}

void nvvkhl::AppBase::createDepthBuffer()
{
  if(m_depthView)
    m_device.destroy(m_depthView);
  if(m_depthImage)
    m_device.destroy(m_depthImage);
  if(m_depthMemory)
    m_device.freeMemory(m_depthMemory);

  // Depth information
  const vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
  vk::ImageCreateInfo        depthStencilCreateInfo;
  depthStencilCreateInfo.setImageType(vk::ImageType::e2D);
  depthStencilCreateInfo.setExtent(vk::Extent3D{m_size.width, m_size.height, 1});
  depthStencilCreateInfo.setFormat(m_depthFormat);
  depthStencilCreateInfo.setMipLevels(1);
  depthStencilCreateInfo.setArrayLayers(1);
  depthStencilCreateInfo.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc);
  // Create the depth image
  m_depthImage = m_device.createImage(depthStencilCreateInfo);

  // Allocate the memory
  const vk::MemoryRequirements memReqs = m_device.getImageMemoryRequirements(m_depthImage);
  vk::MemoryAllocateInfo       memAllocInfo;
  memAllocInfo.allocationSize  = memReqs.size;
  memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
  m_depthMemory                = m_device.allocateMemory(memAllocInfo);

  // Bind image and memory
  m_device.bindImageMemory(m_depthImage, m_depthMemory, 0);

  // Create an image barrier to change the layout from undefined to DepthStencilAttachmentOptimal
  vk::CommandBuffer cmdBuffer = m_device.allocateCommandBuffers({m_cmdPool, vk::CommandBufferLevel::ePrimary, 1})[0];
  cmdBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  // Put barrier on top, Put barrier inside setup command buffer
  vk::ImageSubresourceRange subresourceRange;
  subresourceRange.aspectMask = aspect;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = 1;
  vk::ImageMemoryBarrier imageMemoryBarrier;
  imageMemoryBarrier.oldLayout               = vk::ImageLayout::eUndefined;
  imageMemoryBarrier.newLayout               = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  imageMemoryBarrier.image                   = m_depthImage;
  imageMemoryBarrier.subresourceRange        = subresourceRange;
  imageMemoryBarrier.srcAccessMask           = vk::AccessFlags();
  imageMemoryBarrier.dstAccessMask           = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
  const vk::PipelineStageFlags srcStageMask  = vk::PipelineStageFlagBits::eTopOfPipe;
  const vk::PipelineStageFlags destStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;

  cmdBuffer.pipelineBarrier(srcStageMask, destStageMask, vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
  cmdBuffer.end();
  m_queue.submit(vk::SubmitInfo{0, nullptr, nullptr, 1, &cmdBuffer}, vk::Fence());
  m_queue.waitIdle();
  m_device.freeCommandBuffers(m_cmdPool, cmdBuffer);

  // Setting up the view
  vk::ImageViewCreateInfo depthStencilView;
  depthStencilView.setViewType(vk::ImageViewType::e2D);
  depthStencilView.setFormat(m_depthFormat);
  depthStencilView.setSubresourceRange(subresourceRange);
  depthStencilView.setImage(m_depthImage);
  m_depthView = m_device.createImageView(depthStencilView);
}

void nvvkhl::AppBase::prepareFrame()
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
  while(m_device.waitForFences(m_waitFences[imageIndex], VK_TRUE, 10000) == vk::Result::eTimeout)
  {
  }

  // start new frame with updated camera
  updateCamera();
}

void nvvkhl::AppBase::submitFrame()
{
  uint32_t imageIndex = m_swapChain.getActiveImageIndex();
  m_device.resetFences(m_waitFences[imageIndex]);

  // In case of using NVLINK
  const uint32_t                deviceMask  = m_useNvlink ? 0b0000'0011 : 0b0000'0001;
  const std::array<uint32_t, 2> deviceIndex = {0, 1};

  vk::DeviceGroupSubmitInfo deviceGroupSubmitInfo;
  deviceGroupSubmitInfo.setWaitSemaphoreCount(1);
  deviceGroupSubmitInfo.setCommandBufferCount(1);
  deviceGroupSubmitInfo.setPCommandBufferDeviceMasks(&deviceMask);
  deviceGroupSubmitInfo.setSignalSemaphoreCount(m_useNvlink ? 2 : 1);
  deviceGroupSubmitInfo.setPSignalSemaphoreDeviceIndices(deviceIndex.data());
  deviceGroupSubmitInfo.setPWaitSemaphoreDeviceIndices(deviceIndex.data());

  vk::Semaphore semaphoreRead  = m_swapChain.getActiveReadSemaphore();
  vk::Semaphore semaphoreWrite = m_swapChain.getActiveWrittenSemaphore();

  // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
  const vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  // The submit info structure specifies a command buffer queue submission batch
  vk::SubmitInfo submitInfo;
  submitInfo.setPWaitDstStageMask(&waitStageMask);  // Pointer to the list of pipeline stages that the semaphore waits will occur at
  submitInfo.setPWaitSemaphores(&semaphoreRead);  // Semaphore(s) to wait upon before the submitted command buffer starts executing
  submitInfo.setWaitSemaphoreCount(1);               // One wait semaphore
  submitInfo.setPSignalSemaphores(&semaphoreWrite);  // Semaphore(s) to be signaled when command buffers have completed
  submitInfo.setSignalSemaphoreCount(1);             // One signal semaphore
  submitInfo.setPCommandBuffers(&m_commandBuffers[imageIndex]);  // Command buffers(s) to execute in this batch (submission)
  submitInfo.setCommandBufferCount(1);                           // One command buffer
  submitInfo.setPNext(&deviceGroupSubmitInfo);

  // Submit to the graphics queue passing a wait fence
  m_queue.submit(submitInfo, m_waitFences[imageIndex]);

  // Presenting frame
  m_swapChain.present(m_queue);
}

void nvvkhl::AppBase::setViewport(const vk::CommandBuffer& cmdBuf)
{
  cmdBuf.setViewport(0, {vk::Viewport(0.0f, 0.0f, static_cast<float>(m_size.width), static_cast<float>(m_size.height), 0.0f, 1.0f)});
  cmdBuf.setScissor(0, {{{0, 0}, {m_size.width, m_size.height}}});
}

void nvvkhl::AppBase::onFramebufferSize(int w, int h)
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
  m_device.waitIdle();
  m_queue.waitIdle();

  // Request new swapschain image size
  m_size = m_swapChain.update(m_size.width, m_size.height, m_vsync);

  if(m_size.width != w || m_size.height != h)
  {
    LOGW("Requested size (%d, %d) is different from created size (%u, %u) ", w, h, m_size.width, m_size.height);
  }

  CameraManip.setWindowSize(m_size.width, m_size.height);
  // Invoking Sample callback
  onResize(m_size.width, m_size.height);
  // Recreating other resources
  createDepthBuffer();
  createFrameBuffers();
}

void nvvkhl::AppBase::onMouseMotion(int x, int y)
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

void nvvkhl::AppBase::onKeyboard(int key, int, int action, int mods)
{
  const bool pressed = action != GLFW_RELEASE;

  if(pressed && key == GLFW_KEY_F10)
  {
    m_show_gui = !m_show_gui;
  }
  else if(pressed && key == GLFW_KEY_ESCAPE)
  {
    glfwSetWindowShouldClose(m_window, 1);
  }

  // Remember all keys that are simultaneously pressed for animating the camera
  if(pressed)
  {
    m_keys.insert(key);
  }
  else
  {
    m_keys.erase(key);
  }

  // Keeping track of the modifiers
  m_inputs.ctrl  = mods & GLFW_MOD_CONTROL;
  m_inputs.shift = mods & GLFW_MOD_SHIFT;
  m_inputs.alt   = mods & GLFW_MOD_ALT;
}

void nvvkhl::AppBase::onKeyboardChar(unsigned char key)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
  {
    return;
  }

  // Toggling vsync
  if(key == 'v')
  {
    m_vsync = !m_vsync;
    m_device.waitIdle();
    m_queue.waitIdle();
    m_swapChain.update(m_size.width, m_size.height, m_vsync);
    createFrameBuffers();
  }

  if(key == 'h' || key == '?')
    m_showHelp = !m_showHelp;
}

void nvvkhl::AppBase::onMouseButton(int button, int action, int mods)
{
  (void)mods;

  double x, y;
  glfwGetCursorPos(m_window, &x, &y);
  CameraManip.setMousePosition(static_cast<int>(x), static_cast<int>(y));

  m_inputs.lmb = (button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS);
  m_inputs.mmb = (button == GLFW_MOUSE_BUTTON_MIDDLE) && (action == GLFW_PRESS);
  m_inputs.rmb = (button == GLFW_MOUSE_BUTTON_RIGHT) && (action == GLFW_PRESS);
}

void nvvkhl::AppBase::onMouseWheel(int delta)
{
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  CameraManip.wheel(delta > 0 ? 1 : -1, m_inputs);
}

void nvvkhl::AppBase::onFileDrop(const char* filename) {}

void nvvkhl::AppBase::initGUI(uint32_t subpassID)
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

  std::vector<vk::DescriptorPoolSize> poolSize{{vk::DescriptorType::eSampler, 1}, {vk::DescriptorType::eCombinedImageSampler, 1}};
  vk::DescriptorPoolCreateInfo poolInfo{{}, 2, poolSize};
  m_imguiDescPool = m_device.createDescriptorPool(poolInfo);

  // Setup Platform/Renderer backends
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
  auto cmdbuf = m_device.allocateCommandBuffers({m_cmdPool, vk::CommandBufferLevel::ePrimary, 1})[0];
  cmdbuf.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  ImGui_ImplVulkan_CreateFontsTexture(cmdbuf);
  cmdbuf.end();
  m_queue.submit(vk::SubmitInfo{0, nullptr, nullptr, 1, &cmdbuf}, vk::Fence());
  m_device.waitIdle();
}

void nvvkhl::AppBase::fitCamera(const nvmath::vec3f& boxMin, const nvmath::vec3f& boxMax, bool instantFit)
{
  CameraManip.fit(boxMin, boxMax, instantFit, false, m_size.width / static_cast<float>(m_size.height));
}

bool nvvkhl::AppBase::isMinimized(bool doSleeping)
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

void nvvkhl::AppBase::setTitle(const std::string& title)
{
  glfwSetWindowTitle(m_window, title.c_str());
}

void nvvkhl::AppBase::setupGlfwCallbacks(GLFWwindow* window)
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

void nvvkhl::AppBase::framebuffersize_cb(GLFWwindow* window, int w, int h)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  app->onFramebufferSize(w, h);
}

void nvvkhl::AppBase::mousebutton_cb(GLFWwindow* window, int button, int action, int mods)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  app->onMouseButton(button, action, mods);
}

void nvvkhl::AppBase::cursorpos_cb(GLFWwindow* window, double x, double y)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  app->onMouseMotion(static_cast<int>(x), static_cast<int>(y));
}

void nvvkhl::AppBase::scroll_cb(GLFWwindow* window, double x, double y)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  app->onMouseWheel(static_cast<int>(y));
}

void nvvkhl::AppBase::key_cb(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  app->onKeyboard(key, scancode, action, mods);
}

void nvvkhl::AppBase::char_cb(GLFWwindow* window, unsigned int key)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  app->onKeyboardChar(key);
}

void nvvkhl::AppBase::drop_cb(GLFWwindow* window, int count, const char** paths)
{
  auto app = reinterpret_cast<AppBase*>(glfwGetWindowUserPointer(window));
  int  i;
  for(i = 0; i < count; i++)
    app->onFileDrop(paths[i]);
}

void nvvkhl::AppBase::useNvlink(bool useNvlink)
{
  m_useNvlink = useNvlink;
}

vk::Instance nvvkhl::AppBase::getInstance()
{
  return m_instance;
}

vk::Device nvvkhl::AppBase::getDevice()
{
  return m_device;
}

vk::PhysicalDevice nvvkhl::AppBase::getPhysicalDevice()
{
  return m_physicalDevice;
}

vk::Queue nvvkhl::AppBase::getQueue()
{
  return m_queue;
}

uint32_t nvvkhl::AppBase::getQueueFamily()
{
  return m_graphicsQueueIndex;
}

vk::CommandPool nvvkhl::AppBase::getCommandPool()
{
  return m_cmdPool;
}

vk::RenderPass nvvkhl::AppBase::getRenderPass()
{
  return m_renderPass;
}

vk::Extent2D nvvkhl::AppBase::getSize()
{
  return m_size;
}

vk::PipelineCache nvvkhl::AppBase::getPipelineCache()
{
  return m_pipelineCache;
}

vk::SurfaceKHR nvvkhl::AppBase::getSurface()
{
  return m_surface;
}

const std::vector<vk::Framebuffer>& nvvkhl::AppBase::getFramebuffers()
{
  return m_framebuffers;
}

const std::vector<vk::CommandBuffer>& nvvkhl::AppBase::getCommandBuffers()
{
  return m_commandBuffers;
}

uint32_t nvvkhl::AppBase::getCurFrame() const
{
  return m_swapChain.getActiveImageIndex();
}

vk::Format nvvkhl::AppBase::getColorFormat() const
{
  return m_colorFormat;
}

vk::Format nvvkhl::AppBase::getDepthFormat() const
{
  return m_depthFormat;
}

bool nvvkhl::AppBase::showGui()
{
  return m_show_gui;
}

uint32_t nvvkhl::AppBase::getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const
{
  auto deviceMemoryProperties = m_physicalDevice.getMemoryProperties();
  for(uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
  {
    if(((typeBits & (1 << i)) > 0) && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }
  LOGE("Unable to find memory type %u\n", static_cast<unsigned int>(properties));
  assert(0);
  return ~0u;
}

void nvvkhl::AppBase::uiDisplayHelp()
{
  if(m_showHelp)
  {
    ImGui::BeginChild("Help", ImVec2(370, 120), true);
    ImGui::Text("%s", CameraManip.getHelp().c_str());
    ImGui::EndChild();
  }
}

void nvvkhl::AppBase::updateCamera()
{
  // measure one frame at a time
  float factor = static_cast<float>(m_timer.elapsed());
  m_timer.reset();

  // Allow camera movement only when not editing
  if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard)
    return;

  // For all pressed keys - apply the action
  CameraManip.keyMotion(0, 0, nvh::CameraManipulator::NoAction);
  for(auto key : m_keys)
  {
    switch(key)
    {
      case GLFW_KEY_W:
        CameraManip.keyMotion(factor, 0, nvh::CameraManipulator::Dolly);
        break;
      case GLFW_KEY_S:
        CameraManip.keyMotion(-factor, 0, nvh::CameraManipulator::Dolly);
        break;
      case GLFW_KEY_A:
      case GLFW_KEY_LEFT:
        CameraManip.keyMotion(-factor, 0, nvh::CameraManipulator::Pan);
        break;
      case GLFW_KEY_UP:
        CameraManip.keyMotion(0, factor, nvh::CameraManipulator::Pan);
        break;
      case GLFW_KEY_D:
      case GLFW_KEY_RIGHT:
        CameraManip.keyMotion(factor, 0, nvh::CameraManipulator::Pan);
        break;
      case GLFW_KEY_DOWN:
        CameraManip.keyMotion(0, -factor, nvh::CameraManipulator::Pan);
        break;
      default:
        break;
    }
  }

  // This makes the camera to transition smoothly to the new position
  CameraManip.updateAnim();
}
