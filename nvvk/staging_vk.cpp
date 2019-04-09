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

#include <algorithm>

#include "physical_vk.hpp"
#include "staging_vk.hpp"

namespace nvvk
{

  void FixedSizeStagingBuffer::init(VkDevice                            device,
                                const VkPhysicalDeviceMemoryProperties* memoryProperties,
                                VkDeviceSize                            chunkSize,
                                const VkAllocationCallbacks*            allocator /*= nullptr*/)
  {
    m_device           = device;
    m_allocator        = allocator;
    m_memoryProperties = memoryProperties;
    m_chunkSize        = chunkSize;
    m_available        = 0;
    m_used             = 0;
    m_buffer           = nullptr;
    m_mapping          = nullptr;
    m_mem              = nullptr;

    allocateBuffer(m_chunkSize);
  }

  void FixedSizeStagingBuffer::deinit()
  {
    if(m_available)
    {
      vkUnmapMemory(m_device, m_mem);
      vkDestroyBuffer(m_device, m_buffer, m_allocator);
      vkFreeMemory(m_device, m_mem, m_allocator);
      m_buffer    = nullptr;
      m_mapping   = nullptr;
      m_mem       = nullptr;
      m_available = 0;
    }
    m_targetBuffers.clear();
    m_targetBufferCopies.clear();
    m_used = 0;
  }

  void FixedSizeStagingBuffer::allocateBuffer(VkDeviceSize size)
  {
    VkResult result;
    // Create staging buffer
    VkBufferCreateInfo bufferStageInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferStageInfo.size               = size;
    bufferStageInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferStageInfo.flags              = 0;

    result = vkCreateBuffer(m_device, &bufferStageInfo, m_allocator, &m_buffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memReqs);

    VkMemoryAllocateInfo memInfo;
    PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(*m_memoryProperties, memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memInfo);

    result = vkAllocateMemory(m_device, &memInfo, nullptr, &m_mem);
    result = vkBindBufferMemory(m_device, m_buffer, m_mem, 0);
    result = vkMapMemory(m_device, m_mem, 0, size, 0, (void**)&m_mapping);

    m_available = size;
    m_used      = 0;
  }

  void FixedSizeStagingBuffer::enqueue(VkImage                         image,
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

      allocateBuffer(std::max(size, m_chunkSize));
    }

    memcpy(m_mapping + m_used, data, size);

    VkBufferImageCopy cpy;
    cpy.bufferOffset      = m_used;
    cpy.bufferRowLength   = 0;
    cpy.bufferImageHeight = 0;
    cpy.imageSubresource  = subresource;
    cpy.imageOffset       = offset;
    cpy.imageExtent       = extent;

    m_targetImages.push_back(image);
    m_targetImageCopies.push_back(cpy);

    m_used += size;
  }

  void FixedSizeStagingBuffer::enqueue(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
  {
    if(m_used + size > m_available)
    {
      assert(m_used == 0 && "forgot to flush prior enqueue");

      if(m_available)
      {
        deinit();
      }

      allocateBuffer(std::max(size, m_chunkSize));
    }

    memcpy(m_mapping + m_used, data, size);

    VkBufferCopy cpy;
    cpy.size      = size;
    cpy.srcOffset = m_used;
    cpy.dstOffset = offset;

    m_targetBuffers.push_back(buffer);
    m_targetBufferCopies.push_back(cpy);

    m_used += size;
  }

  void FixedSizeStagingBuffer::flush(VkCommandBuffer cmd)
  {
    for(size_t i = 0; i < m_targetImages.size(); i++)
    {
      vkCmdCopyBufferToImage(cmd, m_buffer, m_targetImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &m_targetImageCopies[i]);
    }
    // TODO could sort by target buffers and use regions etc...
    for(size_t i = 0; i < m_targetBuffers.size(); i++)
    {
      vkCmdCopyBuffer(cmd, m_buffer, m_targetBuffers[i], 1, &m_targetBufferCopies[i]);
    }

    m_targetImages.clear();
    m_targetImageCopies.clear();
    m_targetBuffers.clear();
    m_targetBufferCopies.clear();
    m_used = 0;
  }

}
