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

#include <algorithm>
#include <assert.h>

#include "buffers_vk.hpp"


namespace nvvk {

VkBuffer createBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
{
  VkResult           result;
  VkBuffer           buffer;
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size               = size;
  bufferInfo.usage              = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.flags              = flags;

  result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
  assert(result == VK_SUCCESS);

  return buffer;
}

VkBufferView createBufferView(VkDevice device, VkBuffer buffer, VkFormat format, VkDeviceSize size, VkDeviceSize offset, VkBufferViewCreateFlags flags)
{
  VkResult result;

  VkBufferViewCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
  info.buffer                 = buffer;
  info.flags                  = flags;
  info.offset                 = offset;
  info.range                  = size;
  info.format                 = format;

  assert(size);

  VkBufferView view;
  result = vkCreateBufferView(device, &info, nullptr, &view);
  assert(result == VK_SUCCESS);
  return view;
}

VkBufferView createBufferView(VkDevice device, const VkDescriptorBufferInfo& dinfo, VkFormat format, VkBufferViewCreateFlags flags)
{
  VkResult result;

  VkBufferViewCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
  info.buffer                 = dinfo.buffer;
  info.flags                  = flags;
  info.offset                 = dinfo.offset;
  info.range                  = dinfo.range;
  info.format                 = format;

  VkBufferView view;
  result = vkCreateBufferView(device, &info, nullptr, &view);
  assert(result == VK_SUCCESS);
  return view;
}

//////////////////////////////////////////////////////////////////////////

void StagingBuffer::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size)
{
  m_device          = device;
  m_bufferSize      = size;
  m_available       = 0;
  m_used            = 0;
  m_buffer          = nullptr;
  m_mapping         = nullptr;
  m_memory          = nullptr;
  m_memoryTypeIndex = ~0;

  allocateBuffer(m_bufferSize, physicalDevice);
}

void StagingBuffer::deinit()
{
  if(m_available)
  {
    vkUnmapMemory(m_device, m_memory);
    vkDestroyBuffer(m_device, m_buffer, nullptr);
    vkFreeMemory(m_device, m_memory, nullptr);
    m_buffer    = nullptr;
    m_mapping   = nullptr;
    m_memory    = nullptr;
    m_available = 0;
  }
  m_used = 0;
}

void StagingBuffer::allocateBuffer(VkDeviceSize size, VkPhysicalDevice physical)
{
  VkResult result;
  // Create staging buffer
  VkBufferCreateInfo bufferStageInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferStageInfo.size               = size;
  bufferStageInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferStageInfo.flags              = 0;

  result = vkCreateBuffer(m_device, &bufferStageInfo, nullptr, &m_buffer);

  VkMemoryRequirements memReqs;
  vkGetBufferMemoryRequirements(m_device, m_buffer, &memReqs);

  if(physical)
  {
    m_memoryTypeIndex = ~0;

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);

    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Find an available memory type that satisfies the requested properties.
    for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
      if((memReqs.memoryTypeBits & (1 << memoryTypeIndex))
         && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps)
      {
        m_memoryTypeIndex = memoryTypeIndex;
        break;
      }
    }

    assert(m_memoryTypeIndex != ~0);
  }

  VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memInfo.allocationSize       = memReqs.size;
  memInfo.memoryTypeIndex      = m_memoryTypeIndex;

  result = vkAllocateMemory(m_device, &memInfo, nullptr, &m_memory);
  result = vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
  result = vkMapMemory(m_device, m_memory, 0, size, 0, (void**)&m_mapping);

  m_available = size;
  m_used      = 0;
}

void StagingBuffer::cmdToImage(VkCommandBuffer                 cmd,
                               VkImage                         image,
                               const VkOffset3D&               offset,
                               const VkExtent3D&               extent,
                               const VkImageSubresourceLayers& subresource,
                               VkDeviceSize                    size,
                               const void*                     data)
{
  if(m_used + size > m_available)
  {
    assert(m_used == 0 && "forgot to flush prior enqueue");

    if(m_available)
    {
      deinit();
    }

    allocateBuffer(std::max(size, m_bufferSize));
  }

  memcpy(m_mapping + m_used, data, size);

  VkBufferImageCopy cpy;
  cpy.bufferOffset      = m_used;
  cpy.bufferRowLength   = 0;
  cpy.bufferImageHeight = 0;
  cpy.imageSubresource  = subresource;
  cpy.imageOffset       = offset;
  cpy.imageExtent       = extent;

  vkCmdCopyBufferToImage(cmd, m_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);

  m_used += size;
}

void* StagingBuffer::cmdToBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
  if(!size)
  {
    return nullptr;
  }

  if(m_used + size > m_available)
  {
    assert(m_used == 0 && "forgot to flush prior enqueue");

    if(m_available)
    {
      deinit();
    }

    allocateBuffer(std::max(size, m_bufferSize));
  }

  uint8_t* mapping = m_mapping + m_used;

  if(data)
  {
    memcpy(mapping, data, size);
  }

  VkBufferCopy cpy;
  cpy.size      = size;
  cpy.srcOffset = m_used;
  cpy.dstOffset = offset;

  vkCmdCopyBuffer(cmd, m_buffer, buffer, 1, &cpy);

  m_used += size;

  return data ? nullptr : (void*)mapping;
}

void StagingBuffer::flush()
{
  m_used = 0;
}

}  // namespace nvvk
