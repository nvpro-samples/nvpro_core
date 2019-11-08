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

#include <vulkan/vulkan_core.h>
#include <nvvk/images_vk.hpp>
#include <nvvk/memorymanagement_vk.hpp>
#include <vk_mem_alloc.h>


namespace nvvk {

// Objects
struct BufferVma
{
  VkBuffer      buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = nullptr;
};

struct ImageVma
{
  VkImage       image = VK_NULL_HANDLE;
  VmaAllocation allocation = nullptr;
};

struct AccelerationVma
{
  VkAccelerationStructureNV accel = VK_NULL_HANDLE;
  VmaAllocation             allocation = nullptr;
};

//////////////////////////////////////////////////////////////////////////

/**
  # class nvvk::StagingMemoryManagerVma

  This utility class wraps the usage of [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
  to allocate the memory for nvvk::StagingMemoryManager

*/

class StagingMemoryManagerVma : public StagingMemoryManager
{
public:
  StagingMemoryManagerVma(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator memAllocator, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
  {
    init(device, physicalDevice, memAllocator, stagingBlockSize);
  }
  StagingMemoryManagerVma() {}

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator memAllocator, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
  {
    StagingMemoryManager::init(device, physicalDevice, stagingBlockSize);
    m_memAllocator = memAllocator;
  }


protected:
  VmaAllocator                m_memAllocator;
  std::vector<VmaAllocation>  m_blockAllocs;

  VkResult allocBlockMemory(BlockID id, VkDeviceSize size, bool toDevice, Block& block) override
  {
    VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    createInfo.usage = toDevice ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    createInfo.size = size;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = toDevice ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_TO_CPU;

    VkResult result = vmaCreateBuffer(m_memAllocator, &createInfo, &allocInfo, &block.buffer, &m_blockAllocs[id.index], nullptr);
    if (result != VK_SUCCESS) {
      return result;
    }

    result = vmaMapMemory(m_memAllocator, m_blockAllocs[id.index], (void**)&block.mapping);
    return result;
  }
  void freeBlockMemory(BlockID id, const Block& block) override
  {
    vkDestroyBuffer(m_device, block.buffer, nullptr);
    vmaUnmapMemory(m_memAllocator, m_blockAllocs[id.index]);
    vmaFreeMemory(m_memAllocator, m_blockAllocs[id.index]);
  }

  void resizeBlocks(uint32_t num) override
  {
    if(num)
    {
      m_blockAllocs.resize(num);
    }
    else
    {
      m_blockAllocs.clear();
    }
  }
};

//////////////////////////////////////////////////////////////////////////

/**
  # class nvvk::AllocatorVma

  This utility class wraps the usage of [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
  as well as nvvk::StagingMemoryManager to have a simpler interface
  for handling resources with content uploads.

  See more details in description of nvvk::AllocatorDma.

*/

class AllocatorVma
{
public:
  //--------------------------------------------------------------------------------------------------
  // Initialization of the allocator
  void init(VkDevice device, VmaAllocator allocator, nvvk::StagingMemoryManager& staging)
  {
    m_device    = device;
    m_allocator = allocator;
    m_staging   = &staging;
  }

  //--------------------------------------------------------------------------------------------------
  // Basic buffer creation
  BufferVma createBuffer(const VkBufferCreateInfo& info, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    BufferVma resultBuffer;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memUsage;
    VkResult result = vmaCreateBuffer(m_allocator, &info, &allocInfo, &resultBuffer.buffer, &resultBuffer.allocation, nullptr);
    assert(result == VK_SUCCESS);
    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  BufferVma createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    BufferVma resultBuffer;
    VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    createInfo.size = size;
    createInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return createBuffer(createInfo, memUsage);
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  BufferVma createBuffer(VkCommandBuffer       cmd,
                         VkDeviceSize          size,
                         VkBufferUsageFlags    usage,
                         const void*           data     = nullptr,
                         VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {

    BufferVma resultBuffer = createBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memUsage);
    if(data)
    {
      m_staging->cmdToBuffer(cmd, resultBuffer.buffer, 0, size, data);
    }

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  template <typename T>
  BufferVma createBuffer(VkCommandBuffer       cmd,
                         VkDeviceSize          size,
                         VkBufferUsageFlags    usage,
                         const std::vector<T>& data,
                         VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    VkDeviceSize size         = sizeof(T) * data.size();
    BufferVma    resultBuffer = createBuffer(size, usage, memUsage);
    if(data)
    {
      m_staging->cmdToBuffer(cmd, resultBuffer.buffer, 0, size, data.data());
    }

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Basic image creation
  ImageVma createImage(const VkImageCreateInfo& info, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    ImageVma resultImage;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memUsage;
    VkResult result = vmaCreateImage(m_allocator, &info, &allocInfo, &resultImage.image, &resultImage.allocation, nullptr);
    assert(result == VK_SUCCESS);
    return resultImage;
  }

  //--------------------------------------------------------------------------------------------------
  // Create an image with data, data is assumed to be from first level & layer only
  //
  ImageVma createImage(VkCommandBuffer          cmd,
                       const VkImageCreateInfo& info,
                       VkImageLayout            layout,
                       VkDeviceSize             size,
                       const void*              data,
                       VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    ImageVma resultImage = createImage(info, memUsage);

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
  AccelerationVma createAcceleration(VkAccelerationStructureCreateInfoNV& createInfo,
                                     VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    AccelerationVma resultAccel;
    VkResult result;

    VkAccelerationStructureNV accel;
    result = vkCreateAccelerationStructureNV(m_device, &createInfo, nullptr, &accel);
    if(result != VK_SUCCESS)
    {
      return resultAccel;
    }

    VkMemoryRequirements2                           memReqs = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    VkAccelerationStructureMemoryRequirementsInfoNV memInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
    memInfo.accelerationStructure = accel;
    vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memInfo, &memReqs);

    VmaAllocation allocation = nullptr;
    VmaAllocationInfo allocationDetail;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memUsage;
    result = vmaAllocateMemory(m_allocator, &memReqs.memoryRequirements, &allocInfo, &allocation, &allocationDetail);

    if(result != VK_SUCCESS)
    {
      vkDestroyAccelerationStructureNV(m_device, accel, nullptr);
      return resultAccel;
    }

    VkBindAccelerationStructureMemoryInfoNV bind = {VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV};
    bind.accelerationStructure                   = accel;
    bind.memory                                  = allocationDetail.deviceMemory;
    bind.memoryOffset                            = allocationDetail.offset;

    assert(allocationDetail.offset % memReqs.memoryRequirements.alignment == 0);

    result = vkBindAccelerationStructureMemoryNV(m_device, 1, &bind);
    if(result != VK_SUCCESS)
    {
      vkDestroyAccelerationStructureNV(m_device, accel, nullptr);
      vmaFreeMemory(m_allocator, allocation);
      return resultAccel;
    }

    resultAccel.accel = accel;
    resultAccel.allocation = allocation;
    return resultAccel;
  }

  //--------------------------------------------------------------------------------------------------
  // implicit staging operations triggered by create are managed here

  StagingID finalizeStaging(VkFence fence = VK_NULL_HANDLE) { return m_staging->finalizeCmds(fence); }
  void      releaseStaging(StagingID stagingID) {m_staging->release(stagingID);}
  void      tryReleaseFencedStaging() {m_staging->tryReleaseFenced();}

  //--------------------------------------------------------------------------------------------------
  // Destroy
  //
  void destroy(BufferVma& buffer)
  {
    if(buffer.buffer)
    {
      vkDestroyBuffer(m_device, buffer.buffer, nullptr);
    }
    if(buffer.allocation)
    {
      vmaFreeMemory(m_allocator, buffer.allocation);
    }

    buffer = BufferVma();
  }

  void destroy(ImageVma& image)
  {
    if(image.image)
    {
      vkDestroyImage(m_device, image.image, nullptr);
    }
    if(image.allocation)
    {
      vmaFreeMemory(m_allocator, image.allocation);
    }

    image = ImageVma();
  }

  void destroy(AccelerationVma& accel)
  {
    if(accel.accel)
    {
      vkDestroyAccelerationStructureNV(m_device, accel.accel, nullptr);
    }
    if(accel.allocation)
    {
      vmaFreeMemory(m_allocator, accel.allocation);
    }

    accel = AccelerationVma();
  }

  //--------------------------------------------------------------------------------------------------
  // Other
  //
  void* map(const BufferVma& buffer)
  {
    void* mapped;
    vmaMapMemory(m_allocator, buffer.allocation, &mapped);
    return mapped;
  }
  void unmap(const BufferVma& buffer) { vmaUnmapMemory(m_allocator, buffer.allocation); }


private:
  VkDevice                    m_device;
  VmaAllocator                m_allocator;
  nvvk::StagingMemoryManager* m_staging;
};

}  // namespace nvvk
