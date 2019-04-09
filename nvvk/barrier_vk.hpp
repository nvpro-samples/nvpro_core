/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include <assert.h>
#include <platform.h>
#include <vector>
#include <vulkan/vulkan.h>


namespace nvvk {

  
  // default setup of image barrier for all layers/miplevels
  void setupImageMemoryBarrier(VkImageMemoryBarrier& barrier,
    VkImage img, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

  // swaps dst/src AccesMask and queueFamilyIndex and old/newLayout
  void reverseImageMemoryBarrier(VkImageMemoryBarrier& barrier);


  // TODO potentially BufferState/MemoryState?
  //
  // Memroy-wide barriers are typically sufficient on most hardware.

  //////////////////////////////////////////////////////////////////////////

  class ImageState 
  {
    /*
     
     Class that tracks the access/layout state of an image and allows to easily create barriers/transitions
     when necessary.
    
     After "init", make use of "transition" to add barrier/pipeFlags to vkCmdPipelineBarrier 
    
    */

  private:
    VkImage                   m_image = VK_NULL_HANDLE;
    VkAccessFlags             m_currentAccess = 0;
    VkImageLayout             m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageSubresourceRange   m_subresourceRange = {};

    ImageState() {
      m_subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      m_subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }
  public:
    void   init(VkImage image, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
    {
      m_image = image;
      m_subresourceRange.aspectMask = aspectMask;
      m_currentAccess = 0;
      m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    // always sets up the barrier and flags, but returns true only if barrier is needed
    bool   transition(VkAccessFlags access, VkImageLayout layout, VkImageMemoryBarrier& barrier, VkPipelineStageFlags &pipeSrcFlags, VkPipelineStageFlags &pipeDstFlags);
    // force sets state (danger zone, use in combination with renderPass layout transitions)
    void   setManual(VkAccessFlags access, VkImageLayout layout);
  };

  //////////////////////////////////////////////////////////////////////////

  // TODO BufferState/MemoryState Transitions, maybe single class to cover all

  template <int MAX_BARRIERS>
  class ImageStateTransitions {
  private:
    VkPipelineStageFlags              m_srcPipeFlag = 0;
    VkPipelineStageFlags              m_dstPipeFlag = 0;
    uint32_t                          m_imageBarriersCount = 0;
    VkImageMemoryBarrier              m_imageBarriers[MAX_BARRIERS];

  public:
    void push(ImageState& imgState, VkAccessFlags access, VkImageLayout layout)
    {
      assert(m_imageBarriersCount < MAX_BARRIERS);

      VkPipelineStageFlags srcFlag;
      VkPipelineStageFlags dstFlag;

      if (imgState.transition(access, layout, m_imageBarriers[m_imageBarriersCount], srcFlag, dstFlag)) {
        m_srcPipeFlag |= srcFlag;
        m_dstPipeFlag |= dstFlag;
        m_imageBarriersCount++;
      }
    }

    void clear()
    {
      m_dstPipeFlag = 0;
      m_srcPipeFlag = 0;
      m_imageBarriersCount = 0;
    }

    void cmdPipelineBarriers(VkCommandBuffer cmd, VkDependencyFlags flags)
    {
      if (m_imageBarriersCount) {
        vkCmdPipelineBarrier(cmd, m_srcPipeFlag, m_dstPipeFlag, flags,
          0, nullptr, 0, nullptr, m_imageBarriersCount, m_imageBarriers);
      }
      clear();
    }
  };
}