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

#pragma  once

#include <assert.h>
#include <platform.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "submission_vk.hpp"

namespace nvvk {

  class FixedSizeStagingBuffer
  {
  public:
    // Uses a single memory allocation,
    // therefore operations are only safe
    // once flushed cmd buffer was completed.

    void init(VkDevice                        device,
      const VkPhysicalDeviceMemoryProperties* memoryProperties,
      VkDeviceSize                            chunkSize = 1024 * 1024 * 32,
      const VkAllocationCallbacks*            allocator = nullptr);
    void deinit();

    bool canFlush() const { return m_used != 0; }

    // encodes the vkCmdCopyBuffer and vkCmdCopyBufferToImage commands into the provided buffer
    // and resets the internal state for future enqueue operations
    void flush(VkCommandBuffer cmd);

    // must flush if true
    bool cannotEnqueue(VkDeviceSize sz) const { return (m_used && m_used + sz > m_available); }

    void enqueue(VkImage                         image,
      const VkOffset3D&               offset,
      const VkExtent3D&               extent,
      const VkImageSubresourceLayers& subresource,
      VkDeviceSize                    size,
      const void*                     data);

    void enqueue(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

  private:
    void allocateBuffer(VkDeviceSize size);

    VkBuffer                     m_buffer;
    uint8_t*                     m_mapping;
    VkDeviceSize                 m_used;
    VkDeviceSize                 m_available;
    VkDeviceSize                 m_chunkSize;
    VkDeviceMemory               m_mem;

    std::vector<VkImage>           m_targetImages;
    std::vector<VkBufferImageCopy> m_targetImageCopies;
    std::vector<VkBuffer>          m_targetBuffers;
    std::vector<VkBufferCopy>      m_targetBufferCopies;

    VkDevice                                m_device;
    const VkAllocationCallbacks*            m_allocator;
    const VkPhysicalDeviceMemoryProperties* m_memoryProperties;
  };


}
