/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#include <assert.h>
#include <array>
#include <cstdio>  // printf, fprintf
#include <cstdlib> // abort
#include <set>
#include <vector>
#include <nvh/nvprint.hpp>

#include <vulkan/vulkan.h>
#include <nvvk/deviceutils_vk.hpp>
#include "defaultframe_vk.hpp"

namespace nvvk {

  //--------------------------------------------------------------------------------------------------
  //
  //
  static void check_vk_result(VkResult err)
  {
    if (err == 0)
    {
      return;
    }
    LOGE("VkResult %d\n", err);
    if (err < 0)
    {
      assert(!"Critical Vulkan Error");
    }
  }

  bool DefaultFrame::init(ContextWindowVK* contextWindow, int w, int h, int MSAA)
  {
    m_contextWindow = contextWindow;
    fb_width = w;
    fb_height = h;
    switch (MSAA)
    {
    case 0:
    case 1:
      m_samples = VK_SAMPLE_COUNT_1_BIT;
      break;
    case 2:
      m_samples = VK_SAMPLE_COUNT_2_BIT;
      break;
    case 4:
      m_samples = VK_SAMPLE_COUNT_4_BIT;
      break;
    case 8:
      m_samples = VK_SAMPLE_COUNT_8_BIT;
      break;
    case 16:
      m_samples = VK_SAMPLE_COUNT_16_BIT;
      break;
    case 32:
      m_samples = VK_SAMPLE_COUNT_32_BIT;
      break;
    case 64:
      m_samples = VK_SAMPLE_COUNT_64_BIT;
      break;
    default:
      return false;
    }
    VkDevice device = m_contextWindow->m_context.m_device;
    VkResult err;
    m_numFrames = m_contextWindow->m_swapChain.getSwapchainImageCount();
    assert(m_numFrames <= VK_MAX_QUEUED_FRAMES);
    for (uint32_t i = 0; i < m_numFrames; i++)
    {
      {
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = m_contextWindow->m_presentQueueFamily;
        err = vkCreateCommandPool(device, &info, m_allocator, &m_commandPool[i]);
        check_vk_result(err);
      }
      {
        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = m_commandPool[i];
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;
        err = vkAllocateCommandBuffers(device, &info, &m_commandBuffer[i]);
        check_vk_result(err);
      }
      {
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        err = vkCreateFence(device, &info, m_allocator, &m_fence[i]);
        check_vk_result(err);
      }
      //{
      //  VkSemaphoreCreateInfo info = {};
      //  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      //  err = vkCreateSemaphore(device, &info, m_allocator, &m_presentCompleteSemaphore[i]);
      //  check_vk_result(err);
      //  err = vkCreateSemaphore(m_device, &info, m_allocator, &m_renderCompleteSemaphore[i]);
      //  check_vk_result(err);
      //}
    }
    return true;
  }
  void DefaultFrame::deinit()
  {
    VkDevice device = m_contextWindow->m_context.m_device;
    if(device == VK_NULL_HANDLE)
      return;
    for (uint32_t i = 0; i < m_numFrames; i++)
    {
      vkDestroyFence(device, m_fence[i], m_allocator);
      m_fence[i] = VK_NULL_HANDLE;
      //vkFreeCommandBuffers(device, m_commandPool[i], 1, &m_commandBuffer[i]);
      vkDestroyCommandPool(device, m_commandPool[i], m_allocator);
      m_commandPool[i] = VK_NULL_HANDLE;
      //vkDestroySemaphore(device, m_presentCompleteSemaphore[i], m_allocator);
      //vkDestroySemaphore(device, m_renderCompleteSemaphore[i], m_allocator);
    }
    vkDestroyRenderPass(device, m_renderPass, m_allocator);
    m_renderPass = VK_NULL_HANDLE;

    if (m_depthImageView)        vkDestroyImageView(device, m_depthImageView, nullptr);
    if (m_depthImage)            vkDestroyImage(device, m_depthImage, nullptr);
    if (m_depthImageMemory)      vkFreeMemory(device, m_depthImageMemory, nullptr);
    if (m_msaaColorImageView)    vkDestroyImageView(device, m_msaaColorImageView, nullptr);
    if (m_msaaColorImage)        vkDestroyImage(device, m_msaaColorImage, nullptr);
    if (m_msaaColorImageMemory)  vkFreeMemory(device, m_msaaColorImageMemory, nullptr);
    m_depthImageView = VK_NULL_HANDLE;
    m_depthImage = VK_NULL_HANDLE;
    m_depthImageMemory = VK_NULL_HANDLE;
    m_msaaColorImageView = VK_NULL_HANDLE;
    m_msaaColorImage = VK_NULL_HANDLE;
    m_msaaColorImageMemory = VK_NULL_HANDLE;
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  bool DefaultFrame::resize(int w, int h)
  {
    VkResult err;
    assert(m_contextWindow && (m_contextWindow->m_context.m_device != NULL));
    if((m_contextWindow == NULL) || (m_contextWindow->m_context.m_device == NULL))
      return false;
    VkDevice device = m_contextWindow->m_context.m_device;

    err = vkDeviceWaitIdle(device);
    check_vk_result(err);

    for (uint32_t i = 0; i < m_backBufferCount; i++)
    {
      if (m_framebuffer[i])
      {
        vkDestroyFramebuffer(device, m_framebuffer[i], m_allocator);
      }
    }

    if (m_renderPass)
    {
      vkDestroyRenderPass(device, m_renderPass, m_allocator);
    }

    fb_width = w;
    fb_height = h;

    m_backBufferCount = m_contextWindow->m_swapChain.getImageCount(); // m_pSwapChain->getSwapchainImageCount();
    for (uint32_t i = 0; i < m_backBufferCount; i++)
    {
      m_backBuffer[i] = m_contextWindow->m_swapChain.getImage(i);
      m_backBufferView[i] = m_contextWindow->m_swapChain.getImageView(i);
    }
    m_surfaceFormat = m_contextWindow->m_swapChain.getFormat();

    if(m_depthImageView)        vkDestroyImageView(device, m_depthImageView, nullptr);
    if(m_depthImage)            vkDestroyImage(device, m_depthImage, nullptr);
    if(m_depthImageMemory)      vkFreeMemory(device, m_depthImageMemory, nullptr);
    if(m_msaaColorImageView)    vkDestroyImageView(device, m_msaaColorImageView, nullptr);
    if(m_msaaColorImage)        vkDestroyImage(device, m_msaaColorImage, nullptr);
    if(m_msaaColorImageMemory)  vkFreeMemory(device, m_msaaColorImageMemory, nullptr);
    m_depthImageView = VK_NULL_HANDLE;
    m_depthImage = VK_NULL_HANDLE;
    m_depthImageMemory = VK_NULL_HANDLE;
    m_msaaColorImageView = VK_NULL_HANDLE;
    m_msaaColorImage = VK_NULL_HANDLE;
    m_msaaColorImageMemory = VK_NULL_HANDLE;

    createDepthResources();
    createMSAAColorResources();

    // Create the Render Pass:
    createRenderPass();
    // Create Framebuffer:
    createFrameBuffer();

    return true;
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::frameBegin()
  {
    VkResult err;
    assert(m_contextWindow && (m_contextWindow->m_context.m_device != NULL));
    if ((m_contextWindow == NULL) || (m_contextWindow->m_context.m_device == NULL))
      return;
    VkDevice device = m_contextWindow->m_context.m_device;
    int frameIndex = m_contextWindow->m_swapChain.getActiveImageIndex();

    for (;;)
    {
      err = vkWaitForFences(device, 1, &m_fence[frameIndex], VK_TRUE, 100);
      if (err == VK_SUCCESS)
      {
        break;
      }
      if (err == VK_TIMEOUT)
      {
        continue;
      }
      check_vk_result(err);
    }
    {
      //m_contextWindow->m_swapChain.acquire();
      //err = vkAcquireNextImageKHR(device, m_contextWindow->m_swapChain.getSwapchain(), UINT64_MAX,
      //  m_presentCompleteSemaphore[frameIndex], VK_NULL_HANDLE,
      //  &m_backBufferIndices[frameIndex]);
      //check_vk_result(err);
    }
    {
      // err = vkResetCommandPool(m_device, m_commandPool[frameIndex], 0);
      // check_vk_result(err);
      VkCommandBufferBeginInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      err = vkBeginCommandBuffer(m_commandBuffer[frameIndex], &info);
      check_vk_result(err);
    }
    {
      VkRenderPassBeginInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      info.renderPass = m_renderPass;
      info.framebuffer = m_framebuffer[frameIndex];
      info.renderArea.extent.width = fb_width;
      info.renderArea.extent.height = fb_height;
      std::vector<VkClearValue> clearValues;
      VkClearValue c;
      c.color = m_clearValue.color;
      clearValues.push_back(c);
      c.depthStencil = { 1.0f, 0 };
      clearValues.push_back(c);
      if (m_samples != VK_SAMPLE_COUNT_1_BIT) {
        clearValues.push_back(c);
      }

      info.clearValueCount = static_cast<uint32_t>(clearValues.size());
      info.pClearValues = clearValues.data();
      vkCmdBeginRenderPass(m_commandBuffer[frameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
    }
  }
  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::frameNoMSAANoDST()
  {
    if(m_samples == VK_SAMPLE_COUNT_1_BIT)
      return;
    int frameIndex = m_contextWindow->m_swapChain.getActiveImageIndex();
    vkCmdNextSubpass(m_commandBuffer[frameIndex], VK_SUBPASS_CONTENTS_INLINE);
  }
  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::frameEnd()
  {
    VkResult err;
    VkDevice device = m_contextWindow->m_context.m_device;
    int frameIndex = m_contextWindow->m_swapChain.getActiveImageIndex();
    vkCmdEndRenderPass(m_commandBuffer[frameIndex]);
    {
      VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      VkSubmitInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      info.waitSemaphoreCount = 1;
      VkSemaphore waitSemaphores[1] = { m_contextWindow->m_swapChain.getActiveReadSemaphore() };
      info.pWaitSemaphores = waitSemaphores;// &m_presentCompleteSemaphore[frameIndex];
      info.pWaitDstStageMask = &wait_stage;
      info.commandBufferCount = 1;
      info.pCommandBuffers = &m_commandBuffer[frameIndex];
      info.signalSemaphoreCount = 1;
      VkSemaphore signalSemaphores[1] = { m_contextWindow->m_swapChain.getActiveWrittenSemaphore() };
      info.pSignalSemaphores = signalSemaphores; //&m_renderCompleteSemaphore[frameIndex];

      err = vkEndCommandBuffer(m_commandBuffer[frameIndex]);
      check_vk_result(err);
      err = vkResetFences(device, 1, &m_fence[frameIndex]);
      check_vk_result(err);
      err = vkQueueSubmit(m_contextWindow->m_presentQueue, 1, &info, m_fence[frameIndex]);
      check_vk_result(err);
    }
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  //void DefaultFrame::framePresent()
  //{
  //  //VkResult err;
  //  //int frameIndex = m_contextWindow->m_swapChain.getActiveImageIndex();
  //  //VkSwapchainKHR swapchains[1] = { m_swapchain };
  //  //uint32_t indices[1] = { m_backBufferIndices[frameIndex] };
  //  //VkPresentInfoKHR info = {};
  //  //info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  //  //info.waitSemaphoreCount = 1;
  //  //info.pWaitSemaphores = &m_renderCompleteSemaphore[frameIndex];
  //  //info.swapchainCount = 1;
  //  //info.pSwapchains = swapchains;
  //  //info.pImageIndices = indices;
  //  //err = vkQueuePresentKHR(m_queue, &info);
  //  //check_vk_result(err);
  //  m_contextWindow->m_swapChain.present(m_contextWindow->m_presentQueue);

  //  //m_frameIndex = m_contextWindow->m_swapChain.getActiveImageIndex();//(m_frameIndex + 1) % m_numFrames;
  //}

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::createFrameBuffer()
  {
    VkResult err;
    VkDevice device = m_contextWindow->m_context.m_device;

    int numAttachments = 2;
    int backbufferIndex = 0;
    VkImageView attachments[3] = { m_backBufferView[0], m_depthImageView, VK_NULL_HANDLE };
    if (m_samples != VK_SAMPLE_COUNT_1_BIT)
    {
      numAttachments = 3;
      backbufferIndex = 2;
      attachments[0] = m_msaaColorImageView;
      attachments[2] = m_backBufferView[0];
    }
    ///  VkImageView attachment[1];
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = m_renderPass;
    info.attachmentCount = numAttachments;
    info.pAttachments = attachments;
    info.width = fb_width;
    info.height = fb_height;
    info.layers = 1;
    for (uint32_t i = 0; i < m_backBufferCount; i++)
    {
      attachments[backbufferIndex] = m_backBufferView[i];
      err = vkCreateFramebuffer(device, &info, m_allocator, &m_framebuffer[i]);
      check_vk_result(err);
    }
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::createRenderPass()
  {
    VkResult err;
    VkDevice device = m_contextWindow->m_context.m_device;

    int numAttachments = 2;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_surfaceFormat;
    colorAttachment.samples = m_samples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = m_samples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[2] = {};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &color_attachment;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &color_attachment;
    subpasses[1].pDepthStencilAttachment = NULL;

    // Possible case of MSAA
    VkAttachmentDescription colorResolveAttachment = {};
    colorResolveAttachment.format = m_surfaceFormat;
    colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_resolve_attachment = {};
    color_resolve_attachment.attachment = 2;
    color_resolve_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpasses[0].pResolveAttachments = NULL;
    if (m_samples != VK_SAMPLE_COUNT_1_BIT)
    {
      numAttachments = 3;
      subpasses[0].pResolveAttachments = &color_resolve_attachment;
      subpasses[1].pColorAttachments = &color_resolve_attachment;
    }

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[3] = { colorAttachment, depthAttachment, colorResolveAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = numAttachments;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = (m_samples != VK_SAMPLE_COUNT_1_BIT) ? 2 : 1;
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    err = vkCreateRenderPass(device, &renderPassInfo, m_allocator, &m_renderPass);
    check_vk_result(err);
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  VkFormat DefaultFrame::findSupportedFormat(const std::vector<VkFormat> &candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features)
  {
    for (VkFormat format : candidates)
    {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_contextWindow->m_context.m_physicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
      {
        return format;
      }

      if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
      {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  VkFormat DefaultFrame::findDepthFormat()
  {
    return findSupportedFormat(
    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  bool DefaultFrame::hasStencilComponent(VkFormat format)
  {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::createDepthResources()
  {
    nvvk::DeviceUtilsEx dux(m_contextWindow->m_context.m_device, m_contextWindow->m_context.m_physicalDevice,
                                     m_contextWindow->m_presentQueue, m_contextWindow->m_presentQueueFamily, nullptr);
    VkFormat                     depthFormat = findDepthFormat();
    dux.createImage(fb_width, fb_height, m_samples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_depthImage, m_depthImageMemory);
    m_depthImageView = dux.createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    dux.transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  }
  //--------------------------------------------------------------------------------------------------
  //
  //
  void DefaultFrame::createMSAAColorResources()
  {
    if(m_samples == VK_SAMPLE_COUNT_1_BIT)
      return;
    nvvk::DeviceUtilsEx dux(m_contextWindow->m_context.m_device, m_contextWindow->m_context.m_physicalDevice,
                                     m_contextWindow->m_presentQueue, m_contextWindow->m_presentQueueFamily,
                                     nullptr);
    dux.createImage(fb_width, fb_height, m_samples, m_surfaceFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_msaaColorImage, m_msaaColorImageMemory);
    m_msaaColorImageView = dux.createImageView(m_msaaColorImage, m_surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    dux.transitionImageLayout(m_msaaColorImage, m_surfaceFormat, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  }
  
} //namespace
