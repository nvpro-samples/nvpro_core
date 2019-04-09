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

#include "makers_vk.hpp"

namespace nvvk {

  VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
  {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.flags = flags;

    return bufferInfo;
  }

  VkBufferViewCreateInfo makeBufferViewCreateInfo(VkDescriptorBufferInfo& descrInfo, VkFormat fmt, VkBufferViewCreateFlags flags)
  {
    VkBufferViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    createInfo.buffer = descrInfo.buffer;
    createInfo.offset = descrInfo.offset;
    createInfo.range = descrInfo.range;
    createInfo.flags = flags;
    createInfo.format = fmt;

    return createInfo;
  }
  
  uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask)
  {
    static const uint32_t accessPipes[] = {
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
      VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
      VK_ACCESS_INDEX_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_UNIFORM_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
      | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
      | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
      | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_ACCESS_TRANSFER_READ_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_HOST_READ_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,
      VK_ACCESS_HOST_WRITE_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      0,
      VK_ACCESS_MEMORY_WRITE_BIT,
      0,
      VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX,
      VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX,
      VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX,
      VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX,
    };

    if (!accessMask)
    {
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    uint32_t pipes = 0;

    for (uint32_t i = 0; i < NV_ARRAY_SIZE(accessPipes); i += 2)
    {
      if (accessPipes[i] & accessMask)
      {
        pipes |= accessPipes[i + 1];
      }
    }
    assert(pipes != 0);

    return pipes;
  }

  VkImageMemoryBarrier makeImageMemoryBarrier(VkImage img, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
  {
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
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

    return barrier;
  }

  VkImageMemoryBarrier makeImageMemoryBarrierReversed(const VkImageMemoryBarrier& barrierIn)
  {
    VkImageMemoryBarrier barrier = barrierIn;
    std::swap(barrier.oldLayout, barrier.newLayout);
    std::swap(barrier.srcAccessMask, barrier.dstAccessMask);
    std::swap(barrier.dstQueueFamilyIndex, barrier.srcQueueFamilyIndex);
    return barrier;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    const VkDescriptorBufferInfo*       pBufferInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.pBufferInfo = pBufferInfo;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    const VkDescriptorImageInfo*        pImageInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.pImageInfo = pImageInfo;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    const VkBufferView*                 pTexelBufferView)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.pTexelBufferView = pTexelBufferView;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    const void*                         pNext)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.pNext = pNext;
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    uint32_t                            arrayElement,
    const VkDescriptorBufferInfo*       pBufferInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstArrayElement = arrayElement;
        writeSet.dstSet = dstSet;
        writeSet.pBufferInfo = pBufferInfo;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    uint32_t                            arrayElement,
    const VkDescriptorImageInfo*        pImageInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.dstArrayElement = arrayElement;
        writeSet.pImageInfo = pImageInfo;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
          || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    uint32_t                            arrayElement,
    const VkBufferView*                 pTexelBufferView)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.dstArrayElement = arrayElement;
        writeSet.pTexelBufferView = pTexelBufferView;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet makeWriteDescriptorSet(size_t                              numBindings,
    const VkDescriptorSetLayoutBinding* bindings,
    VkDescriptorSet                     dstSet,
    uint32_t                            dstBinding,
    uint32_t                            arrayElement,
    const void*                         pNext)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++)
    {
      if (bindings[i].binding == dstBinding)
      {
        writeSet.pNext = pNext;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.dstArrayElement = arrayElement;
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

}


