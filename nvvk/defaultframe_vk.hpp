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

#ifndef NV_VK_DEFAULTDEFAULTFRAME_INCLUDED
#define NV_VK_DEFAULTDEFAULTFRAME_INCLUDED


#include <stdio.h>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <nvvk\contextwindow_vk.hpp>

#define VK_MAX_QUEUED_FRAMES 4
#define MAX_POSSIBLE_BACK_BUFFERS 16

namespace nvvk
{
  /*
    DefaultFrame is a basic implementation of whatever is required to have a regular color+Depthstencil setup for the window
    This class is *not mandatory* for a sample to run. It's just a convenient way to have something put together for quick
    rendering in a window
    - a render-pass associated with the framebuffer(s)
    - buffers/framebuffers associated with the views of the window
    - command-buffers to match the current swapchain index
    typical use :
        ...swapPrepare()...
    1)  m_defaultFrame.setClearValue(cv);
        m_defaultFrame.frameBegin();
        ...
    2)  VkCommandBuffer cmdBuff = m_defaultFrame.getCommandBuffer()[m_defaultFrame.getFrameIndex()];
        vkCmd...()
        ...
    3)  //for MSAA case: advances in the sub-pass to render *after* the resolve of AA
        m_defaultFrame.frameNoMSAANoDST();
        ... draw some non MSAA stuff (UI...)
    4)  m_defaultFrame.frameEnd();
        ... swapbuffers()...
  */
  class DefaultFrame {
  private:
    // contains Vulkan device info related to the window : device, surface, swapchain, present-queue, window size
    ContextWindowVK* m_contextWindow;

    // framebuffer size and # of samples
    int             fb_width = 0, fb_height = 0;
    VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

    VkClearValue    m_clearValue = {};

    uint32_t        m_numFrames = 0;
    VkFormat        m_surfaceFormat;
    uint32_t        m_backBufferCount = 0;
    VkRenderPass    m_renderPass = VK_NULL_HANDLE;

    VkCommandPool   m_commandPool[VK_MAX_QUEUED_FRAMES];
    VkCommandBuffer m_commandBuffer[VK_MAX_QUEUED_FRAMES];
    VkFence         m_fence[VK_MAX_QUEUED_FRAMES];

    VkImage         m_backBuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};
    VkImageView     m_backBufferView[MAX_POSSIBLE_BACK_BUFFERS] = {};
    VkFramebuffer   m_framebuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};

    VkImage         m_depthImage = {};
    VkImage         m_msaaColorImage = {};
    VkDeviceMemory  m_depthImageMemory = {};
    VkDeviceMemory  m_msaaColorImageMemory = {};
    VkImageView     m_depthImageView = {};
    VkImageView     m_msaaColorImageView = {};

    VkAllocationCallbacks   *m_allocator = VK_NULL_HANDLE;

    bool        hasStencilComponent(VkFormat format);

  public:
    bool init(ContextWindowVK* contextWindow, int w, int h, int MSAA);
    void deinit();
    bool resize(int w, int h);
    void createFrameBuffer();
    //void createImageViews();
    void createRenderPass();
    void frameBegin();
    void frameNoMSAANoDST();
    void frameEnd();

    void createDepthResources();
    void createMSAAColorResources();
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    void setClearValue(const VkClearValue &clear_value)
    {
      m_clearValue = clear_value;
    }
    uint32_t getFrameIndex()
    {
      return m_contextWindow->m_swapChain.getActiveImageIndex();
    }
    const VkRenderPass &getRenderPass()
    {
      return m_renderPass;
    }
    const int getRenderPassIndexNoMSAANoDST()
    {
      return (m_samples == VK_SAMPLE_COUNT_1_BIT) ? 0 : 1;
    }
    VkFormat getSurfaceFormat() const
    {
      return m_surfaceFormat;
    }

    VkImage getCurrentBackBuffer() const
    {
      return m_backBuffer[m_contextWindow->m_swapChain.getActiveImageIndex()];
    }
    VkCommandBuffer *getCommandBuffer()
    {
      return m_commandBuffer;
    }
  };

} //namespace nvvk

#endif