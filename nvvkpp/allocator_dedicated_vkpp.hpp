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

#include <vulkan/vulkan.hpp>

#include "images_vkpp.hpp"


//////////////////////////////////////////////////////////////////////////
/**
 This is the allocator specialization using only Vulkan where there will be one memory
 allocation for each buffer or image.
 See [allocatorVMA](#Allocators-in-nvvkpp) for details on how to use the allocators.

> Note: this should be used only when really needed, as it is making one allocation per buffer,
>       which is not efficient. 

# Initialization

~~~~~~ C++
    nvvkpp::AllocatorVk m_alloc;
    m_alloc.init(device, physicalDevice);
~~~~~~ 

# AllocatorVkExport
This version of the allocator will export all memory allocations, which can then be use by Cuda or OpenGL.

*/

namespace nvvkpp {

// Objects
struct BufferDedicated
{
  vk::Buffer       buffer;
  vk::DeviceMemory allocation;
};

struct ImageDedicated
{
  vk::Image        image;
  vk::DeviceMemory allocation;
};

struct TextureDedicated : public ImageDedicated
{
  vk::DescriptorImageInfo descriptor;

  TextureDedicated& operator=(const ImageDedicated& buffer)
  {
    static_cast<ImageDedicated&>(*this) = buffer;
    return *this;
  }
};

struct AccelerationDedicated
{
  vk::AccelerationStructureNV accel;
  vk::DeviceMemory            allocation;
};


//--------------------------------------------------------------------------------------------------
// Allocator for buffers, images and acceleration structure using Pure Vulkan
//
class AllocatorDedicated
{
public:
  // All staging buffers must be cleared before
  ~AllocatorDedicated() { assert(m_stagingBuffers.empty()); }

  //--------------------------------------------------------------------------------------------------
  // Initialization of the allocator
  void init(vk::Device device, vk::PhysicalDevice physicalDevice)
  {
    m_device           = device;
    m_physicalDevice   = physicalDevice;
    m_memoryProperties = m_physicalDevice.getMemoryProperties();
  }

  //--------------------------------------------------------------------------------------------------
  // Basic buffer creation
  virtual BufferDedicated createBuffer(const vk::BufferCreateInfo&   info_,
                                       const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    BufferDedicated resultBuffer;
    // 1. Create Buffer
    resultBuffer.buffer = m_device.createBuffer(info_);

    // 2. Find memory requirements
    auto r =
        m_device.getBufferMemoryRequirements2<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>(resultBuffer.buffer);
    vk::MemoryRequirements2         req2    = r.get<vk::MemoryRequirements2>();
    vk::MemoryDedicatedRequirements d       = r.get<vk::MemoryDedicatedRequirements>();
    vk::MemoryRequirements&         memReqs = req2.memoryRequirements;

    // 3. Allocate memory
    vk::MemoryAllocateInfo memAlloc;
    memAlloc.setAllocationSize(memReqs.size);
    memAlloc.setMemoryTypeIndex(getMemoryType(memReqs.memoryTypeBits, memUsage_));
    resultBuffer.allocation = AllocateMemory(memAlloc);
    checkMemory(resultBuffer.allocation);

    // 4. Bind memory to buffer
    m_device.bindBufferMemory(resultBuffer.buffer, resultBuffer.allocation, 0);

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  BufferDedicated createBuffer(vk::DeviceSize                size_     = 0,
                               vk::BufferUsageFlags          usage_    = vk::BufferUsageFlags(),
                               const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return createBuffer({{}, size_, usage_}, memUsage_);
  }


  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  BufferDedicated createBuffer(const vk::CommandBuffer&    cmdBuf,
                               const vk::DeviceSize&       size_  = 0,
                               const void*                 data_  = nullptr,
                               const vk::BufferUsageFlags& usage_ = vk::BufferUsageFlags())
  {
    // 1. Create staging buffer
    BufferDedicated stageBuffer = createBuffer(size_, vk::BufferUsageFlagBits::eTransferSrc,
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_stagingBuffers.push_back(stageBuffer);  // Remember the buffers to delete

    // 2. Copy the data to memory
    if(data_)
    {
      void* mapped = m_device.mapMemory(stageBuffer.allocation, 0, size_, vk::MemoryMapFlags());
      memcpy(mapped, data_, size_);
      m_device.unmapMemory(stageBuffer.allocation);
    }

    // 3. Create the result buffer
    vk::BufferCreateInfo createInfoR{{}, size_, usage_ | vk::BufferUsageFlagBits::eTransferDst};
    BufferDedicated      resultBuffer = createBuffer(createInfoR);

    // 4. Copy staging (need to submit command buffer, flushStaging must be done after submitting)
    cmdBuf.copyBuffer(stageBuffer.buffer, resultBuffer.buffer, vk::BufferCopy(0, 0, size_));

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  template <typename T>
  BufferDedicated createBuffer(const vk::CommandBuffer&    cmdBuff,
                               const std::vector<T>&       data_,
                               const vk::BufferUsageFlags& usage_ = vk::BufferUsageFlags())
  {
    return createBuffer(cmdBuff, sizeof(T) * data_.size(), data_.data(), usage_);
  }


  //--------------------------------------------------------------------------------------------------
  // Basic image creation
  ImageDedicated createImage(const vk::ImageCreateInfo&    info_,
                             const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    ImageDedicated resultImage;
    // 1. Create image
    resultImage.image = m_device.createImage(info_);

    // 2. Find memory requirements
    auto r = m_device.getImageMemoryRequirements2<vk::MemoryRequirements2, vk::MemoryDedicatedRequirements>(resultImage.image);
    vk::MemoryRequirements2         req2    = r.get<vk::MemoryRequirements2>();
    vk::MemoryDedicatedRequirements d       = r.get<vk::MemoryDedicatedRequirements>();
    vk::MemoryRequirements&         memReqs = req2.memoryRequirements;

    // 3. Allocate memory
    vk::MemoryAllocateInfo memAllocInfo;
    memAllocInfo.setAllocationSize(memReqs.size);
    memAllocInfo.setMemoryTypeIndex(getMemoryType(memReqs.memoryTypeBits, memUsage_));
    resultImage.allocation = AllocateMemory(memAllocInfo);
    checkMemory(resultImage.allocation);

    // 4. Bind memory to image
    m_device.bindImageMemory(resultImage.image, resultImage.allocation, 0);

    return resultImage;
  }


  //--------------------------------------------------------------------------------------------------
  // Create an image with data
  //
  ImageDedicated createImage(const vk::CommandBuffer&   cmdBuff,
                             size_t                     size_,
                             const void*                data_,
                             const vk::ImageCreateInfo& info_,
                             const vk::ImageLayout&     layout_ = vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    ImageDedicated resultImage = createImage(info_);

    // Copy the data to staging buffer than to image
    if(data_ != nullptr)
    {
      BufferDedicated stageBuffer =
          createBuffer(size_, vk::BufferUsageFlagBits::eTransferSrc,
                       vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      m_stagingBuffers.push_back(stageBuffer);  // Remember the buffers to delete

      // Copy data to buffer
      void* mapped = m_device.mapMemory(stageBuffer.allocation, 0, size_, vk::MemoryMapFlags());
      memcpy(mapped, data_, size_);
      m_device.unmapMemory(stageBuffer.allocation);

      // Copy buffer to image
      vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eColor, 0, info_.mipLevels, 0, 1);
      nvvkpp::image::setImageLayout(cmdBuff, resultImage.image, vk::ImageLayout::eUndefined,
                                    vk::ImageLayout::eTransferDstOptimal, subresourceRange);
      vk::BufferImageCopy bufferCopyRegion;
      bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
      bufferCopyRegion.imageSubresource.layerCount = 1;
      bufferCopyRegion.imageExtent                 = info_.extent;
      cmdBuff.copyBufferToImage(stageBuffer.buffer, resultImage.image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegion);

      // Setting final image layout
      subresourceRange.levelCount = 1;
      nvvkpp::image::setImageLayout(cmdBuff, resultImage.image, vk::ImageLayout::eTransferDstOptimal, layout_, subresourceRange);
    }
    else
    {
      // Setting final image layout
      nvvkpp::image::setImageLayout(cmdBuff, resultImage.image, vk::ImageLayout::eUndefined, layout_);
    }

    return resultImage;
  }


  //--------------------------------------------------------------------------------------------------
  // Create the acceleration structure
  //
  AccelerationDedicated createAcceleration(vk::AccelerationStructureCreateInfoNV& accel_)
  {
    AccelerationDedicated resultAccel;
    // 1. Create the acceleration structure
    resultAccel.accel = m_device.createAccelerationStructureNV(accel_);

    // 2. Find memory requirements
    vk::AccelerationStructureMemoryRequirementsInfoNV memInfo;
    memInfo.accelerationStructure   = resultAccel.accel;
    vk::MemoryRequirements2 memReqs = m_device.getAccelerationStructureMemoryRequirementsNV(memInfo);

    // 3. Allocate memory
    vk::MemoryAllocateInfo memAlloc;
    memAlloc.setAllocationSize(memReqs.memoryRequirements.size);
    memAlloc.setMemoryTypeIndex(getMemoryType(memReqs.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    resultAccel.allocation = AllocateMemory(memAlloc);

    // 4. Bind memory with acceleration structure
    vk::BindAccelerationStructureMemoryInfoNV bind;
    bind.setAccelerationStructure(resultAccel.accel);
    bind.setMemory(resultAccel.allocation);
    bind.setMemoryOffset(0);
    m_device.bindAccelerationStructureMemoryNV(bind);

    return resultAccel;
  }

  //--------------------------------------------------------------------------------------------------
  // Flushing staging buffers, must be done after the command buffer is submitted
  void flushStaging(vk::Fence fence = vk::Fence())
  {
    if(!m_stagingBuffers.empty())
    {
      m_garbageBuffers.push_back({fence, m_stagingBuffers});
      m_stagingBuffers.clear();
    }
    cleanGarbage();
  }


  //--------------------------------------------------------------------------------------------------
  // Destroy
  //
  void destroy(BufferDedicated& b_)
  {
    m_device.destroyBuffer(b_.buffer);
    m_device.freeMemory(b_.allocation);
  }

  void destroy(ImageDedicated& i_)
  {
    m_device.destroyImage(i_.image);
    m_device.freeMemory(i_.allocation);
  }

  void destroy(AccelerationDedicated& a_)
  {
    m_device.destroyAccelerationStructureNV(a_.accel);
    m_device.freeMemory(a_.allocation);
  }

  void destroy(TextureDedicated& t_)
  {
    m_device.destroyImageView(t_.descriptor.imageView);
    m_device.destroySampler(t_.descriptor.sampler);
    destroy(static_cast<ImageDedicated>(t_));
  }


  //--------------------------------------------------------------------------------------------------
  // Other
  //
  void* map(const BufferDedicated& buffer_) { return m_device.mapMemory(buffer_.allocation, 0, VK_WHOLE_SIZE); }
  void  unmap(const BufferDedicated& buffer_) { m_device.unmapMemory(buffer_.allocation); }


protected:
  // This is to allow Exportable Memory
  virtual vk::DeviceMemory AllocateMemory(vk::MemoryAllocateInfo& allocateInfo)
  {
    return m_device.allocateMemory(allocateInfo);
  }

  void checkMemory(const VkDeviceMemory& memory)
  {
    // If there is a leak in a DeviceMemory allocation, set the ID here to catch the object
    assert(PtrToInt(memory) != 0x00);
  }


  //--------------------------------------------------------------------------------------------------
  // Finding the memory type for memory allocation
  //
  uint32_t getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties)
  {
    for(uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
    {
      if(((typeBits & (1 << i)) > 0) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
      {
        return i;
      }
    }
    assert(0);
    return ~0u;
  }

  // Clean all staging buffers, only if the associated fence is set to ready
  void cleanGarbage()
  {
    auto s = m_garbageBuffers.begin();  // Loop over all garbage
    while(s != m_garbageBuffers.end())
    {
      vk::Result result = vk::Result ::eSuccess;
      if(s->fence)  // Could be that no fence was set
        result = m_device.getFenceStatus(s->fence);
      if(result == vk::Result::eSuccess)
      {
        for(auto& st : s->stagingBuffers)
        {  // Delete all buffers and free up memory
          m_device.destroy(st.buffer);
          m_device.free(st.allocation);
        }
        s = m_garbageBuffers.erase(s);  // Done with it
      }
      else
      {
        ++s;
      }
    }
  }


  struct GarbageCollection
  {
    vk::Fence                    fence;
    std::vector<BufferDedicated> stagingBuffers;
  };
  std::vector<GarbageCollection> m_garbageBuffers;


  vk::Device                         m_device;
  vk::PhysicalDevice                 m_physicalDevice;
  vk::PhysicalDeviceMemoryProperties m_memoryProperties;
  std::vector<BufferDedicated>       m_stagingBuffers;
};  // namespace nvvkpp

//--------------------------------------------------------------------------------------------------
// This class will export all memory allocations, to be used by OpenGL and Cuda Interop
//
class AllocatorVkExport : public AllocatorDedicated
{
protected:
  // Override the standard allocation
  vk::DeviceMemory AllocateMemory(vk::MemoryAllocateInfo& allocateInfo) override
  {
    vk::ExportMemoryAllocateInfo memoryHandleEx(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32);
    allocateInfo.setPNext(&memoryHandleEx);  // <-- Enabling Export
    return m_device.allocateMemory(allocateInfo);
  }
};

//--------------------------------------------------------------------------------------------------
// This class will export all memory allocations, to be used by OpenGL and Cuda Interop
//
class AllocatorExplicitDeviceMask : public AllocatorDedicated
{
public:
  // Initialization of the allocator
  void init(vk::Device device, vk::PhysicalDevice physicalDevice, uint32_t deviceMask)
  {
    m_device           = device;
    m_physicalDevice   = physicalDevice;
    m_memoryProperties = m_physicalDevice.getMemoryProperties();
    m_deviceMask = deviceMask;
  }

protected:
  // Override the standard allocation
  vk::DeviceMemory AllocateMemory(vk::MemoryAllocateInfo& allocateInfo) override
  {
    vk::MemoryAllocateFlagsInfo flags = {};


    flags.setDeviceMask(m_deviceMask);
    flags.setFlags(vk::MemoryAllocateFlagBits::eDeviceMask);


    allocateInfo.setPNext(&flags);  // <-- Enabling Export
    return m_device.allocateMemory(allocateInfo);
  }

  // Target device (first device per default)
  uint32_t m_deviceMask{1u};
};

}  // namespace nvvkpp
