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

#include "barrier_vk.hpp"

#include "makers_vk.hpp"

namespace nvvk
{

  void setupImageMemoryBarrier(VkImageMemoryBarrier& barrier, VkImage img, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask /*= VK_IMAGE_ASPECT_COLOR_BIT*/)
  {
    barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = img;
    barrier.subresourceRange = { 0 };
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
  }

  void reverseImageMemoryBarrier(VkImageMemoryBarrier& barrier)
  {
    std::swap(barrier.oldLayout, barrier.newLayout);
    std::swap(barrier.srcAccessMask, barrier.dstAccessMask);
    std::swap(barrier.dstQueueFamilyIndex, barrier.srcQueueFamilyIndex);
  }
  
  //////////////////////////////////////////////////////////////////////////

  bool ImageState::transition(VkAccessFlags access, VkImageLayout layout, VkImageMemoryBarrier& barrier, VkPipelineStageFlags &pipeSrcFlags, VkPipelineStageFlags &pipeDstFlags)
  {
    bool required = access != m_currentAccess || layout != m_currentLayout;

    pipeSrcFlags = nvvk::makeAccessMaskPipelineStageFlags(m_currentAccess);
    pipeDstFlags = nvvk::makeAccessMaskPipelineStageFlags(access);

    barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = m_currentAccess;
    barrier.dstAccessMask = access;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = layout;
    barrier.image = m_image;
    barrier.subresourceRange = m_subresourceRange;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    m_currentAccess = access;
    m_currentLayout = layout;

    return required;
  }
  
  void ImageState::setManual(VkAccessFlags access, VkImageLayout layout)
  {
    m_currentAccess = access;
    m_currentLayout = layout;
  }

}
