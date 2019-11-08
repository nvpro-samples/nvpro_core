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

#pragma once

#include <platform.h>

#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/**
  The utilities in this file provide a more direct approach, we encourage to use
  higher-level mechanisms also provided in the allocator / memorymanagement classes.

  # functions in nvvk

  - makeBufferCreateInfo : wraps setup of VkBufferCreateInfo (implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT)
  - makeBufferViewCreateInfo : wraps setup of VkBufferViewCreateInfo
  - createBuffer : wraps vkCreateBuffer (implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT)
  - createBufferView : wraps vkCreateBufferView

  ~~~ C++
  VkBufferCreateInfo bufferCreate =   makeBufferCreateInfo (256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  ~~~
*/

// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0);

VkBufferViewCreateInfo makeBufferViewCreateInfo(const VkDescriptorBufferInfo& descrInfo, VkFormat fmt, VkBufferViewCreateFlags flags = 0);

//////////////////////////////////////////////////////////////////////////

// implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
VkBuffer createBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0);

VkBufferView createBufferView(VkDevice                device,
                              VkBuffer                buffer,
                              VkFormat                format,
                              VkDeviceSize            size,
                              VkDeviceSize            offset = 0,
                              VkBufferViewCreateFlags flags  = 0);
VkBufferView createBufferView(VkDevice device, const VkDescriptorBufferInfo& info, VkFormat format, VkBufferViewCreateFlags flags = 0);


//////////////////////////////////////////////////////////////////////////
/**
# class nvvk::StagingBuffer

Stage uploads to images and buffers using this *host visible buffer*.
After init, use `enqueue` operations for uploads, then execute via flushing
the commands to a commandbuffer that you execute.May need to use cannotEnqueue if
the allocated size is too small.Enqueues greater than initial bufferSize *will cause re - allocation * .

*You must synchronize explicitly!*

A single buffer and memory allocation is used, therefore new copy operations are
only safe once copy commands prior flush were completed.

> Look at the nvvk::StagingMemoryManager for a more sophisticated version
> that manages multiple staging buffers and allows easier use of asynchronous
> transfers.

Example:
~~~ C++
StagingBuffer staging;
staging.init(device, physicalDevice, 128 * 1024 * 1024);

staging.cmdToBuffer(cmd,targetBuffer, 0, targetSize, targetData);
staging.flush();

... submit cmd ...

// later once completed

staging.deinit();

~~~
*/

class StagingBuffer
{
public:
  static const VkDeviceSize DEFAULT_BLOCKSIZE = 1024 * 1024 * 64;

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize = DEFAULT_BLOCKSIZE);
  void deinit();

  bool canFlush() const { return m_used != 0; }

  // reset internal state for new copy commmands
  void flush();

  // must flush if true
  bool doesNotFit(VkDeviceSize sz) const { return (m_used && m_used + sz > m_available); }

  void cmdToImage(VkCommandBuffer                 cmd,
                  VkImage                         image,
                  const VkOffset3D&               offset,
                  const VkExtent3D&               extent,
                  const VkImageSubresourceLayers& subresource,
                  VkDeviceSize                    size,
                  const void*                     data);

  // if data != nullptr memcpies to mapping returns nullptr
  // otherwise returns mapping (valid until flush)
  void* cmdToBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

  template <class T>
  T* cmdToBufferT(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
  {
    return (T*)cmdToBuffer(cmd, buffer, offset, size, nullptr);
  }

private:
  void allocateBuffer(VkDeviceSize size, VkPhysicalDevice physicalDevice = VK_NULL_HANDLE);

  VkBuffer       m_buffer          = VK_NULL_HANDLE;
  VkDeviceSize   m_bufferSize      = 0;
  uint8_t*       m_mapping         = nullptr;
  VkDeviceSize   m_used            = 0;
  VkDeviceSize   m_available       = 0;
  VkDeviceMemory m_memory          = VK_NULL_HANDLE;
  uint32_t       m_memoryTypeIndex = ~0;

  VkDevice m_device = VK_NULL_HANDLE;
};


class ScopeStagingBuffer : public StagingBuffer {
public:
  ScopeStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize = StagingBuffer::DEFAULT_BLOCKSIZE)
  {
    init(device, physicalDevice, bufferSize);
  }

  ~ScopeStagingBuffer() {
    deinit();
  }
};

//////////////////////////////////////////////////////////////////////////

NV_INLINE VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
{
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size               = size;
  bufferInfo.usage              = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.flags              = flags;

  return bufferInfo;
}

NV_INLINE VkBufferViewCreateInfo makeBufferViewCreateInfo(const VkDescriptorBufferInfo& descrInfo, VkFormat fmt, VkBufferViewCreateFlags flags)
{
  VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
  createInfo.buffer                 = descrInfo.buffer;
  createInfo.offset                 = descrInfo.offset;
  createInfo.range                  = descrInfo.range;
  createInfo.flags                  = flags;
  createInfo.format                 = fmt;

  return createInfo;
}

}  // namespace nvvk
