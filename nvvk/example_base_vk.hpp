
/* Copyright (c) 2014-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//////////////////////////////////////////////////////////////////////////
/// This is the bas class used by many examples
///


#include <assert.h>
#include <fstream>

#include <vulkan/vulkan.h>

#include <imgui/imgui_helper.h>
#include <imgui/imgui_impl_vk.h>

#include <nvh/cameramanipulator.hpp>
#include <nvh/profiler.hpp>
#include <nvvk/contextwindow_vk.hpp>
#include <nvvk/deviceutils_vk.hpp>
#include <nvvk/error_vk.hpp>
#include <nvmath/nvmath.h>

#include "nvpwindow.hpp"


#define VK_MAX_QUEUED_FRAMES 4
#define MAX_POSSIBLE_BACK_BUFFERS 16

namespace nvvk {

//--------------------------------------------------------------------------------------------------
//
//
class ExampleBase : public NVPWindow
{
public:
  //-----------------------------------------------------------
  // Method in which you create *local* things for the window
  //
  //
  virtual bool create(int posX, int posY, int width, int height, const char* title, const nvvk::ContextInfoVK& context, int MSAA = 1)
  {
    // Window creation
    NVPWindow::create(posX, posY, width, height, title);

    // Vulkan creation for this window, the device and all ...
    m_vkctx.init(&context, this);
    m_vkctx.swapResize(width, height);

    // Keeping Device helper functionalities
    m_vkDevice.init(m_vkctx.m_context.m_device, m_vkctx.m_context.m_physicalDevice, m_vkctx.m_presentQueue,
                    m_vkctx.m_context.m_physicalInfo.getQueueFamily());

    // Creation of all frames
    initFrame(getWidth(), getHeight(), MSAA);
    // Force the creation of the frames
    recreateFrames(width, height);


    // UI
    ImGuiH::Init(width, height, this);
    m_vkDevice.m_device = m_vkctx.m_context.m_device;
    ImGui::InitVK(m_vkDevice, m_vkctx.m_context.m_physicalInfo, getRenderPass(), m_vkctx.m_presentQueue,
                  m_vkctx.m_presentQueueFamily, getRenderPassIndexNoMSAANoDST());
    ImGui::GetIO().IniFilename = nullptr;  // Avoiding the INI file


    // Setup camera
    CameraManip.setWindowSize(getWidth(), getHeight());
    CameraManip.setLookat(nvmath::vec3f(0.0f, 10.0f, 10.0f), nvmath::vec3f(0, 0, 0), nvmath::vec3f(0, 1, 0));


    onInitExample();  // Override function

    return true;
  }

  //--------------------------------------------------------------------------------------------------
  // All examples need to implement those function
  //
  virtual void onInitExample()     = 0;
  virtual void onShutdownExample() = 0;

  //--------------------------------------------------------------------------------------------------
  // Removing all allocated objects
  // Called when the window is closed
  //
  void shutdown() override
  {
    VkDevice device(m_vkctx.m_context.m_device);
    if(device == VK_NULL_HANDLE)
      return;
    CHECK_VK_RESULT(vkDeviceWaitIdle(device));
    onShutdownExample();

    for(uint32_t i = 0; i < m_numFrames; i++)
    {
      vkDestroyFence(device, m_fence[i], m_allocator);
      m_fence[i] = VK_NULL_HANDLE;
      vkDestroyCommandPool(device, m_commandPool[i], m_allocator);
      m_commandPool[i] = VK_NULL_HANDLE;
      vkDestroyFramebuffer(device, m_framebuffer[i], m_allocator);
      m_framebuffer[i] = VK_NULL_HANDLE;
    }

    vkDestroyRenderPass(device, m_renderPass, m_allocator);
    m_renderPass = VK_NULL_HANDLE;

    if(m_depthImageView)
      vkDestroyImageView(device, m_depthImageView, nullptr);
    if(m_depthImage)
      vkDestroyImage(device, m_depthImage, nullptr);
    if(m_depthImageMemory)
      vkFreeMemory(device, m_depthImageMemory, nullptr);
    if(m_msaaColorImageView)
      vkDestroyImageView(device, m_msaaColorImageView, nullptr);
    if(m_msaaColorImage)
      vkDestroyImage(device, m_msaaColorImage, nullptr);
    if(m_msaaColorImageMemory)
      vkFreeMemory(device, m_msaaColorImageMemory, nullptr);
    m_depthImageView       = VK_NULL_HANDLE;
    m_depthImage           = VK_NULL_HANDLE;
    m_depthImageMemory     = VK_NULL_HANDLE;
    m_msaaColorImageView   = VK_NULL_HANDLE;
    m_msaaColorImage       = VK_NULL_HANDLE;
    m_msaaColorImageMemory = VK_NULL_HANDLE;

    ImGui::ShutdownVK();
    VK_CHECK(vkDeviceWaitIdle(m_vkctx.m_context.m_device));
    ImGui::DestroyContext();

    m_vkDevice.deInit();
    m_vkctx.deinit();
  }

  //------------------------------------------------------
  // Called by the window resize event (WM_SIZE in Windows)
  //
  virtual void reshape(int w = 0, int h = 0) override
  {
    if(w == 0)
      w = m_windowSize[0];
    if(h == 0)
      h = m_windowSize[1];
    m_vkctx.swapResize(w, h);
    recreateFrames(w, h);
    {
      //...
      CameraManip.setWindowSize(w, h);
    }
    ImGui::ReInitPipelinesVK(m_vkDevice, getRenderPass(), getRenderPassIndexNoMSAANoDST());
  }
  //------------------------------------------------------
  // Keyboard events
  //
  virtual void keyboard(NVPWindow::KeyCode key, ButtonAction action, int mods, int x, int y) override
  {
    m_inputs.ctrl  = (key == KEY_LEFT_CONTROL) && (action == NVPWindow::BUTTON_PRESS);
    m_inputs.shift = (key == KEY_LEFT_SHIFT) && (action == NVPWindow::BUTTON_PRESS);
    m_inputs.alt   = (key == KEY_LEFT_ALT) && (action == NVPWindow::BUTTON_PRESS);

    if(action == NVPWindow::BUTTON_RELEASE)
      return;
    switch(key)
    {
      case NVPWindow::KEY_ESCAPE:
        sysPostQuit();
        break;
    }
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  virtual void keyboardchar(unsigned char key, int mods, int x, int y) override
  {
    switch(key)
    {
      case '`':
      case 'u':
        m_bUseUI ^= 1;
        break;
    }
  }

  //------------------------------------------------------
  // Mouse move events
  //
  void motion(int x, int y) override
  {
    if(ImGuiH::mouse_pos(x, y))
      return;

    if(m_inputs.lmb || m_inputs.rmb || m_inputs.mmb)
    {
      CameraManip.mouseMove(x, y, m_inputs);
    }
  }
  //--------------------------------------------------------------------------------------------------
  // Mouse button
  //
  void mouse(NVPWindow::MouseButton button, NVPWindow::ButtonAction state, int mods, int x, int y) override
  {
    if(ImGuiH::mouse_button(button, state))
      return;

    CameraManip.setMousePosition(x, y);

    m_inputs.lmb = (button == NVPWindow::MOUSE_BUTTON_LEFT) && (state == NVPWindow::BUTTON_PRESS);
    m_inputs.mmb = (button == NVPWindow::MOUSE_BUTTON_MIDDLE) && (state == NVPWindow::BUTTON_PRESS);
    m_inputs.rmb = (button == NVPWindow::MOUSE_BUTTON_RIGHT) && (state == NVPWindow::BUTTON_PRESS);
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void mousewheel(int delta) override
  {
    if(ImGuiH::mouse_wheel(delta))
      return;

    CameraManip.wheel(delta > 0 ? 1 : -1, m_inputs);
  }

  //--------------------------------------------------------------------------------------------------
  // Recreation of all frame buffer and render pass
  //
  virtual void recreateFrames(int w, int h)
  {
    assert(m_vkctx.m_context.m_device != nullptr);
    if(m_vkctx.m_context.m_device == nullptr)
      return;
    VkDevice device = m_vkctx.m_context.m_device;

    VK_CHECK(vkDeviceWaitIdle(device));

    for(uint32_t i = 0; i < m_backBufferCount; i++)
    {
      if(m_framebuffer[i])
      {
        vkDestroyFramebuffer(device, m_framebuffer[i], m_allocator);
      }
    }

    if(m_renderPass)
    {
      vkDestroyRenderPass(device, m_renderPass, m_allocator);
    }

    m_width  = w;
    m_height = h;

    m_backBufferCount = m_vkctx.m_swapChain.getImageCount();  // m_pSwapChain->getSwapchainImageCount();
    for(uint32_t i = 0; i < m_backBufferCount; i++)
    {
      m_backBuffer[i]     = m_vkctx.m_swapChain.getImage(i);
      m_backBufferView[i] = m_vkctx.m_swapChain.getImageView(i);
    }
    m_surfaceFormat = m_vkctx.m_swapChain.getFormat();

    if(m_depthImageView)
      vkDestroyImageView(device, m_depthImageView, nullptr);
    if(m_depthImage)
      vkDestroyImage(device, m_depthImage, nullptr);
    if(m_depthImageMemory)
      vkFreeMemory(device, m_depthImageMemory, nullptr);
    if(m_msaaColorImageView)
      vkDestroyImageView(device, m_msaaColorImageView, nullptr);
    if(m_msaaColorImage)
      vkDestroyImage(device, m_msaaColorImage, nullptr);
    if(m_msaaColorImageMemory)
      vkFreeMemory(device, m_msaaColorImageMemory, nullptr);
    m_depthImageView       = VK_NULL_HANDLE;
    m_depthImage           = VK_NULL_HANDLE;
    m_depthImageMemory     = VK_NULL_HANDLE;
    m_msaaColorImageView   = VK_NULL_HANDLE;
    m_msaaColorImage       = VK_NULL_HANDLE;
    m_msaaColorImageMemory = VK_NULL_HANDLE;

    createDepthResources();
    createMSAAColorResources();

    createRenderPass();
    createFrameBuffer();

    return;
  }

  //--------------------------------------------------------------------------------------------------
  // Creation of all frame buffers, check for MSAA
  //
  void createFrameBuffer()
  {
    VkDevice device = m_vkctx.m_context.m_device;

    int         numAttachments  = 2;
    int         backbufferIndex = 0;
    VkImageView attachments[3]  = {m_backBufferView[0], m_depthImageView, VK_NULL_HANDLE};
    if(m_samples != VK_SAMPLE_COUNT_1_BIT)
    {
      numAttachments  = 3;
      backbufferIndex = 2;
      attachments[0]  = m_msaaColorImageView;
      attachments[2]  = m_backBufferView[0];
    }

    VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    info.renderPass      = m_renderPass;
    info.attachmentCount = numAttachments;
    info.pAttachments    = attachments;
    info.width           = m_width;
    info.height          = m_height;
    info.layers          = 1;
    for(uint32_t i = 0; i < m_backBufferCount; i++)
    {
      attachments[backbufferIndex] = m_backBufferView[i];
      VK_CHECK(vkCreateFramebuffer(device, &info, m_allocator, &m_framebuffer[i]));
    }
  }

  //--------------------------------------------------------------------------------------------------
  // Default render pass, with depth buffer and MSAA support
  //
  virtual void createRenderPass()
  {
    VkDevice device = m_vkctx.m_context.m_device;

    int numAttachments = 2;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format                  = m_surfaceFormat;
    colorAttachment.samples                 = m_samples;
    colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_attachment  = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentDescription depthAttachment  = {};
    depthAttachment.format                   = findDepthFormat();
    depthAttachment.samples                  = m_samples;
    depthAttachment.loadOp                   = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp            = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp           = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout              = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpasses[3]    = {};
    subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount    = 1;
    subpasses[0].pColorAttachments       = &color_attachment;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

    subpasses[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount    = 1;
    subpasses[1].pColorAttachments       = &color_attachment;
    subpasses[1].pDepthStencilAttachment = nullptr;

    subpasses[2].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[2].colorAttachmentCount    = 1;
    subpasses[2].pColorAttachments       = &color_attachment;
    subpasses[2].pDepthStencilAttachment = nullptr;


    // Possible case of MSAA
    VkAttachmentDescription colorResolveAttachment = {};
    colorResolveAttachment.format                  = m_surfaceFormat;
    colorResolveAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    colorResolveAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorResolveAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    colorResolveAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorResolveAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorResolveAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    colorResolveAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_resolve_attachment = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    subpasses[0].pResolveAttachments = nullptr;
    if(m_samples != VK_SAMPLE_COUNT_1_BIT)
    {
      numAttachments                   = 3;
      subpasses[0].pResolveAttachments = &color_resolve_attachment;
      subpasses[1].pColorAttachments   = &color_resolve_attachment;
      subpasses[2].pColorAttachments   = &color_resolve_attachment;
    }

    std::vector<VkSubpassDependency> dependencies;
    VkSubpassDependency              dependency = {};
    dependency.srcSubpass                       = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass                       = 0;
    dependency.srcStageMask                     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask                     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask                    = 0;
    dependency.dstAccessMask                    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.push_back(dependency);
    dependency.srcSubpass = 0;
    dependency.dstSubpass = 1;
    dependencies.push_back(dependency);
    if(m_samples != VK_SAMPLE_COUNT_1_BIT)
    {
      dependency.srcSubpass = 1;
      dependency.dstSubpass = 2;
      dependencies.push_back(dependency);
    }


    VkAttachmentDescription attachments[3] = {colorAttachment, depthAttachment, colorResolveAttachment};
    VkRenderPassCreateInfo  renderPassInfo = {};
    renderPassInfo.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount         = numAttachments;
    renderPassInfo.pAttachments            = attachments;
    renderPassInfo.subpassCount            = (m_samples != VK_SAMPLE_COUNT_1_BIT) ? 3 : 2;
    renderPassInfo.pSubpasses              = subpasses;
    renderPassInfo.dependencyCount         = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies           = dependencies.data();
    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, m_allocator, &m_renderPass));
  }


  //--------------------------------------------------------------------------------------------------
  //  Function to call at the begining of a new frame
  //
  void frameBegin()
  {
    VkResult err;
    assert(m_vkctx.m_context.m_device != nullptr);
    if(m_vkctx.m_context.m_device == nullptr)
      return;
    VkDevice device     = m_vkctx.m_context.m_device;
    int      frameIndex = m_vkctx.m_swapChain.getActiveImageIndex();

    for(;;)
    {
      err = vkWaitForFences(device, 1, &m_fence[frameIndex], VK_TRUE, 100);
      if(err == VK_SUCCESS)
      {
        break;
      }
      if(err == VK_TIMEOUT)
      {
        continue;
      }
      VK_CHECK(err);
    }
    {
      VkCommandBufferBeginInfo info = {};
      info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      VK_CHECK(vkBeginCommandBuffer(m_commandBuffer[frameIndex], &info));
    }
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void renderBegin(VkCommandBuffer commandBuffer, VkSubpassContents contents)
  {
    int frameIndex = m_vkctx.m_swapChain.getActiveImageIndex();

    VkRenderPassBeginInfo info    = {};
    info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass               = m_renderPass;
    info.framebuffer              = m_framebuffer[frameIndex];
    info.renderArea.extent.width  = m_width;
    info.renderArea.extent.height = m_height;
    std::vector<VkClearValue> clearValues;
    VkClearValue              c;
    c.color = m_clearValue.color;
    clearValues.push_back(c);
    c.depthStencil = {1.0f, 0};
    clearValues.push_back(c);
    if(m_samples != VK_SAMPLE_COUNT_1_BIT)
    {
      clearValues.push_back(c);
    }

    info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    info.pClearValues    = clearValues.data();
    vkCmdBeginRenderPass(m_commandBuffer[frameIndex], &info, contents);
  }

  //--------------------------------------------------------------------------------------------------
  // Skipping the subpass when MSAA is active
  //
  void frameNoMSAANoDST()
  {
    if(m_samples == VK_SAMPLE_COUNT_1_BIT)
      return;
    int frameIndex = m_vkctx.m_swapChain.getActiveImageIndex();
    vkCmdNextSubpass(m_commandBuffer[frameIndex], VK_SUBPASS_CONTENTS_INLINE);
  }

  //--------------------------------------------------------------------------------------------------
  // Sending all command buffers to execution. vkCmdEndRenderPass must be done
  //
  void frameEnd()
  {
    VkDevice device     = m_vkctx.m_context.m_device;
    int      frameIndex = m_vkctx.m_swapChain.getActiveImageIndex();

    {
      VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      VkSubmitInfo         info       = {};
      info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      info.waitSemaphoreCount         = 1;
      VkSemaphore waitSemaphores[1]   = {m_vkctx.m_swapChain.getActiveReadSemaphore()};
      info.pWaitSemaphores            = waitSemaphores;
      info.pWaitDstStageMask          = &wait_stage;
      info.commandBufferCount         = 1;
      info.pCommandBuffers            = &m_commandBuffer[frameIndex];
      info.signalSemaphoreCount       = 1;
      VkSemaphore signalSemaphores[1] = {m_vkctx.m_swapChain.getActiveWrittenSemaphore()};
      info.pSignalSemaphores          = signalSemaphores;

      VK_CHECK(vkEndCommandBuffer(m_commandBuffer[frameIndex]));
      VK_CHECK(vkResetFences(device, 1, &m_fence[frameIndex]));
      VK_CHECK(vkQueueSubmit(m_vkctx.m_presentQueue, 1, &info, m_fence[frameIndex]));
    }
  }

  //--------------------------------------------------------------------------------------------------
  // Creating the Depth image to be attached
  //
  void createDepthResources()
  {
    VkFormat depthFormat = findDepthFormat();
    m_vkDevice.createImage(m_width, m_height, m_samples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
    m_depthImageView = m_vkDevice.createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    m_vkDevice.transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  }

  //--------------------------------------------------------------------------------------------------
  // Creating the image supporting MSAA
  //
  void createMSAAColorResources()
  {
    {
      if(m_samples == VK_SAMPLE_COUNT_1_BIT)
        return;
      m_vkDevice.createImage(m_width, m_height, m_samples, m_surfaceFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_msaaColorImage, m_msaaColorImageMemory);
      m_msaaColorImageView = m_vkDevice.createImageView(m_msaaColorImage, m_surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);

      m_vkDevice.transitionImageLayout(m_msaaColorImage, m_surfaceFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
  }

  //--------------------------------------------------------------------------------------------------
  // Returns the best depth format
  //
  VkFormat findDepthFormat()
  {
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }
  //--------------------------------------------------------------------------------------------------
  // Find the format for the features
  //
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
  {
    for(VkFormat format : candidates)
    {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_vkctx.m_context.m_physicalDevice, format, &props);

      if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
      {
        return format;
      }

      if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
      {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }

  //--------------------------------------------------------------------------------------------------
  // Convenient function to load the shaders (SPIR-V)
  //
  std::vector<char> readFile(const std::string& filename)
  {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
      file = std::ifstream(std::string("../") + filename, std::ios::ate | std::ios::binary);
      if(!file.is_open())
      {
        file = std::ifstream(std::string(PROJECT_ABSDIRECTORY) + filename, std::ios::ate | std::ios::binary);
        if(!file.is_open())
          throw std::runtime_error("failed to open file!");
      }
    }

    auto              fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }


  void                setClearValue(const VkClearValue& clear_value) { m_clearValue = clear_value; }
  uint32_t            getFrameIndex() { return m_vkctx.m_swapChain.getActiveImageIndex(); }
  const VkRenderPass& getRenderPass() { return m_renderPass; }
  const int           getRenderPassIndexNoMSAANoDST() { return (m_samples == VK_SAMPLE_COUNT_1_BIT) ? 1 : 2; }
  VkFormat            getSurfaceFormat() const { return m_surfaceFormat; }
  VkImage             getCurrentBackBuffer() const { return m_backBuffer[m_vkctx.m_swapChain.getActiveImageIndex()]; }
  VkImageView getCurrentBackBufferView() const { return m_backBufferView[m_vkctx.m_swapChain.getActiveImageIndex()]; }
  VkCommandBuffer*      getCommandBuffer() { return m_commandBuffer; }
  VkSampleCountFlagBits getSamples() { return m_samples; }

private:
  bool initFrame(int w, int h, int MSAA)
  {
    m_width   = w;
    m_height  = h;
    m_samples = static_cast<VkSampleCountFlagBits>(MSAA);

    VkDevice device = m_vkctx.m_context.m_device;
    m_numFrames     = m_vkctx.m_swapChain.getSwapchainImageCount();
    assert(m_numFrames <= VK_MAX_QUEUED_FRAMES);
    for(uint32_t i = 0; i < m_numFrames; i++)
    {
      {
        VkCommandPoolCreateInfo info = {};
        info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex        = m_vkctx.m_presentQueueFamily;
        VK_CHECK(vkCreateCommandPool(device, &info, m_allocator, &m_commandPool[i]));
      }
      {
        VkCommandBufferAllocateInfo info = {};
        info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool                 = m_commandPool[i];
        info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount          = 1;
        VK_CHECK(vkAllocateCommandBuffers(device, &info, &m_commandBuffer[i]));
      }
      {
        VkFenceCreateInfo info = {};
        info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &info, m_allocator, &m_fence[i]));
      }
    }
    return true;
  }


protected:
  bool m_bUseUI = true;

  // Vulkan's device/context for rendering VK in this window
  // NOTE: this class is a convenient class for the most common case where
  // we have only one window. it contains all the necessary stuff for VK
  // InstanceDeviceContext class; VkSurfaceKHR; SwapChain class; VkQueue and Family idx for 'presenting'
  // Now, if you have more than 1 window, InstanceDeviceContext would be redundant
  nvvk::ContextWindowVK m_vkctx;

  // Holding the device, the allocator and having the implementation of many utilities
  nvvk::DeviceUtilsEx m_vkDevice;

private:
  // Camera manipulator, like in Maya, 3dsmax, Softimage, ...
  nvh::CameraManipulator::Inputs m_inputs;

  // framebuffer size and # of samples
  uint32_t              m_width   = 0;
  uint32_t              m_height  = 0;
  VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

  VkClearValue m_clearValue = {};

  uint32_t     m_numFrames       = 0;   // Number of frames in the swapchain
  VkFormat     m_surfaceFormat   = {};  // The format, RGBA, R64G64B64A64_SFLOAT, ..
  uint32_t     m_backBufferCount = 0;
  VkRenderPass m_renderPass      = VK_NULL_HANDLE;

  VkCommandPool   m_commandPool[VK_MAX_QUEUED_FRAMES]   = {};
  VkCommandBuffer m_commandBuffer[VK_MAX_QUEUED_FRAMES] = {};
  VkFence         m_fence[VK_MAX_QUEUED_FRAMES]         = {};

  VkImage       m_backBuffer[MAX_POSSIBLE_BACK_BUFFERS]     = {};
  VkImageView   m_backBufferView[MAX_POSSIBLE_BACK_BUFFERS] = {};
  VkFramebuffer m_framebuffer[MAX_POSSIBLE_BACK_BUFFERS]    = {};

  VkImage        m_depthImage           = {};
  VkImage        m_msaaColorImage       = {};
  VkDeviceMemory m_depthImageMemory     = {};
  VkDeviceMemory m_msaaColorImageMemory = {};
  VkImageView    m_depthImageView       = {};
  VkImageView    m_msaaColorImageView   = {};

  VkAllocationCallbacks* m_allocator = VK_NULL_HANDLE;
};

}  // namespace nvvk
