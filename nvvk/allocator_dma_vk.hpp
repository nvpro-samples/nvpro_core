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
#pragma once

#include <nvvk/images_vk.hpp>
#include <nvvk/memorymanagement_vk.hpp>
#include <vulkan/vulkan_core.h>

/**
  # class nvvk::AllocatorDma

  The goal of the `AllocatorABC` classes is to have common work-flow
  even if the underlying allocator classes are different.
  This should make it relatively easy to switch between different
  allocator implementations (more or less only changing typedefs).

  The `BufferABC`, `ImageABC` etc. structs always contain the native
  resource handle as well as the allocator system's handle.

  This utility class wraps the usage of nvvk::DeviceMemoryAllocator
  as well as nvvk::StagingMemoryManager to have a simpler interface
  for handling resources with content uploads.

  > Note: These classes are foremost to showcase principle components that
  > a Vulkan engine would most likely have.
  > They are geared towards ease of use in this sample framework, and 
  > not optimized nor meant for production code.

  ~~~ C++
  AllocatorDma allocator;
  allocator.init(memAllocator, staging);

  ...

  VkCommandBuffer cmd = ... transfer queue command buffer

  // creates new resources and 
  // implicitly triggers staging transfer copy operations into cmd
  BufferDma vbo = allocator.createBuffer(cmd, vboSize, vboUsage, vboData);
  BufferDma ibo = allocator.createBuffer(cmd, iboSize, iboUsage, iboData);

  // use functions from staging memory manager
  // here we associate the temporary staging space with a fence
  allocator.finalizeStaging(fence);

  // submit cmd buffer with staging copy operations
  vkQueueSubmit(... cmd ... fence ...)

  ...

  // if you do async uploads you would
  // trigger garbage collection somewhere per frame
  allocator.tryReleaseFencedStaging();

  ~~~
*/

namespace nvvk {

// Objects
struct BufferDma
{
  VkBuffer     buffer = VK_NULL_HANDLE;
  AllocationID allocation;
};

struct ImageDma
{
  VkImage      image = VK_NULL_HANDLE;
  AllocationID allocation;
};

struct AccelerationDma
{
  VkAccelerationStructureNV accel = VK_NULL_HANDLE;
  AllocationID              allocation;
};


//--------------------------------------------------------------------------------------------------
// Allocator for buffers, images and acceleration structure using Device Memory Allocator
//
class AllocatorDma
{
public:
  //--------------------------------------------------------------------------------------------------
  // Initialization of the allocator
  void init(VkDevice device, nvvk::DeviceMemoryAllocator& allocator, nvvk::StagingMemoryManager& staging)
  {
    m_device    = device;
    m_allocator = &allocator;
    m_staging   = &staging;
  }

  // sets memory priority for VK_EXT_memory_priority
  float setPriority(float priority = nvvk::DeviceMemoryAllocator::DEFAULT_PRIORITY) { return m_allocator->setPriority(priority); }

  //--------------------------------------------------------------------------------------------------
  // Basic buffer creation
  BufferDma createBuffer(const VkBufferCreateInfo& info, const VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    BufferDma resultBuffer;
    resultBuffer.buffer = m_allocator->createBuffer(info, resultBuffer.allocation, memProps);
    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  BufferDma createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    BufferDma resultBuffer;
    resultBuffer.buffer = m_allocator->createBuffer(size, usage, resultBuffer.allocation, memProps);
    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  BufferDma createBuffer(VkCommandBuffer       cmd,
                         VkDeviceSize          size,
                         VkBufferUsageFlags    usage,
                         const void*           data     = nullptr,
                         VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {

    BufferDma resultBuffer = createBuffer(size, usage, memProps);
    if(data)
    {
      m_staging->cmdToBuffer(cmd, resultBuffer.buffer, 0, size, data);
    }

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  template <typename T>
  BufferDma createBuffer(VkCommandBuffer       cmd,
                         VkDeviceSize          size,
                         VkBufferUsageFlags    usage,
                         const std::vector<T>& data,
                         VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    VkDeviceSize size         = sizeof(T) * data.size();
    BufferDma    resultBuffer = createBuffer(size, usage, memProps);
    if(data)
    {
      m_staging->cmdToBuffer(cmd, resultBuffer.buffer, 0, size, data.data());
    }

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Basic image creation
  ImageDma createImage(const VkImageCreateInfo& info, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    ImageDma resultImage;
    resultImage.image = m_allocator->createImage(info, resultImage.allocation, memProps);
    return resultImage;
  }

  //--------------------------------------------------------------------------------------------------
  // Create an image with data, data is assumed to be from first level & layer only
  //
  ImageDma createImage(VkCommandBuffer          cmd,
                       const VkImageCreateInfo& info,
                       VkImageLayout            layout,
                       VkDeviceSize             size,
                       const void*              data,
                       VkMemoryPropertyFlags    memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    ImageDma resultImage;
    resultImage.image = m_allocator->createImage(info, resultImage.allocation, memProps);

    // Copy the data to staging buffer than to image
    if(data != nullptr)
    {
      // doing these transitions per copy is not efficient, should do in bulk for many images

      nvvk::cmdTransitionImage(cmd, resultImage.image, info.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      VkOffset3D               offset      = {0};
      VkImageSubresourceLayers subresource = {0};
      subresource.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
      subresource.layerCount               = 1;

      m_staging->cmdToImage(cmd, resultImage.image, offset, info.extent, subresource, size, data);

      // Setting final image layout
      nvvk::cmdTransitionImage(cmd, resultImage.image, info.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout);
    }
    else
    {
      // Setting final image layout
      nvvk::cmdTransitionImage(cmd, resultImage.image, info.format, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    }

    return resultImage;
  }

  //--------------------------------------------------------------------------------------------------
  // Create the acceleration structure
  //
  AccelerationDma createAcceleration(VkAccelerationStructureCreateInfoNV& accel,
                                     VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    AccelerationDma resultAccel;
    resultAccel.accel = m_allocator->createAccStructure(accel, resultAccel.allocation, memProps);

    return resultAccel;
  }

  //--------------------------------------------------------------------------------------------------
  // implicit staging operations triggered by create are managed here

  StagingID finalizeStaging(VkFence fence = VK_NULL_HANDLE) { return m_staging->finalizeCmds(fence); }
  void      releaseStaging(StagingID stagingID) { m_staging->release(stagingID); }
  void      tryReleaseFencedStaging() { m_staging->tryReleaseFenced(); }

  //--------------------------------------------------------------------------------------------------
  // Destroy
  //
  void destroy(BufferDma& buffer)
  {
    if(buffer.buffer)
    {
      vkDestroyBuffer(m_device, buffer.buffer, nullptr);
    }
    if(buffer.allocation)
    {
      m_allocator->free(buffer.allocation);
    }

    buffer = BufferDma();
  }

  void destroy(ImageDma& image)
  {
    if(image.image)
    {
      vkDestroyImage(m_device, image.image, nullptr);
    }
    if(image.allocation)
    {
      m_allocator->free(image.allocation);
    }

    image = ImageDma();
  }

  void destroy(AccelerationDma& accel)
  {
    if(accel.accel)
    {
      vkDestroyAccelerationStructureNV(m_device, accel.accel, nullptr);
    }
    if(accel.allocation)
    {
      m_allocator->free(accel.allocation);
    }

    accel = AccelerationDma();
  }

  //--------------------------------------------------------------------------------------------------
  // Other
  //
  void* map(const BufferDma& buffer) { return m_allocator->map(buffer.allocation); }
  void  unmap(const BufferDma& buffer) { m_allocator->unmap(buffer.allocation); }


private:
  VkDevice               m_device;
  DeviceMemoryAllocator* m_allocator;
  StagingMemoryManager*  m_staging;
};

}  // namespace nvvk
