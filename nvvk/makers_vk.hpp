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

// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0);
VkBufferViewCreateInfo makeBufferViewCreateInfo(VkDescriptorBufferInfo& descrInfo, VkFormat fmt, VkBufferViewCreateFlags flags = 0);

uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask);

VkImageMemoryBarrier makeImageMemoryBarrier(VkImage            image,
                                            VkAccessFlags      srcAccess,
                                            VkAccessFlags      dstAccess,
                                            VkImageLayout      oldLayout,
                                            VkImageLayout      newLayout,
                                            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

VkImageMemoryBarrier makeImageMemoryBarrierReversed(const VkImageMemoryBarrier& barrier);

// assumes full array provided
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            const VkDescriptorImageInfo*        pImageInfo);
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            const VkDescriptorBufferInfo*       pBufferInfo);
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            const VkBufferView*                 pTexelBufferView);
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            const void*                         pNext);

// single array entry
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            uint32_t                            arrayElement,
                                            const VkDescriptorImageInfo*        pImageInfo);
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            uint32_t                            arrayElement,
                                            const VkDescriptorBufferInfo*       pBufferInfo);
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            uint32_t                            arrayElement,
                                            const VkBufferView*                 pTexelBufferView);
VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
                                            const VkDescriptorSetLayoutBinding* bindings,
                                            VkDescriptorSet                     dstSet,
                                            uint32_t                            dstBinding,
                                            uint32_t                            arrayElement,
                                            const void*                         pNext);

}  // namespace nvvk
