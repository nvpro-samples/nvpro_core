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
#include <nvvk/memorymanagement_vk.hpp>

//////////////////////////////////////////////////////////////////////////
/**
 This is the allocator specialization using: nvvk::DeviceMemoryAllocator.
 See [allocatorVMA](#Allocators-in-nvvkpp) for details on how to use the allocators.

# Initialization

~~~~~~ C++
    nvvk::DeviceMemoryAllocator m_dmaAllocator;
    nvvkpp::AllocatorDma        m_alloc
    m_dmaAllocator.init(device, physicalDevice);
    m_alloc.init(device, &m_dmaAllocator);
~~~~~~ 

*/


namespace nvvkpp {

// Objects
struct BufferDma
{
  vk::Buffer         buffer;
  nvvk::AllocationID allocation{};
};

struct ImageDma
{
  vk::Image          image;
  nvvk::AllocationID allocation{};
};

struct TextureDma : public ImageDma
{
  vk::DescriptorImageInfo descriptor;

  TextureDma& operator=(const ImageDma& buffer)
  {
    static_cast<ImageDma&>(*this) = buffer;
    return *this;
  }
};

struct AccelerationDma
{
  vk::AccelerationStructureNV accel;
  nvvk::AllocationID          allocation{};
};


//--------------------------------------------------------------------------------------------------
// Allocator for buffers, images and acceleration structure using Device Memory Allocator
//
class AllocatorDma
{
public:
  // All staging buffers must be cleared before
  ~AllocatorDma() { assert(m_stagingBuffers.empty()); }

  //--------------------------------------------------------------------------------------------------
  // Initialization of the allocator
  void init(vk::Device device, nvvk::DeviceMemoryAllocator* allocator)
  {
    m_device    = device;
    m_allocator = allocator;
  }

  // sets memory priority for VK_EXT_memory_priority
  float setPriority(float priority = nvvk::DeviceMemoryAllocator::DEFAULT_PRIORITY)
  {
    return m_allocator->setPriority(priority);
  }

  //--------------------------------------------------------------------------------------------------
  // Basic buffer creation
  BufferDma createBuffer(const vk::BufferCreateInfo&   info_,
                         const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    BufferDma             resultBuffer;
    VkMemoryPropertyFlags flags{memUsage_};
    resultBuffer.buffer = m_allocator->createBuffer(info_, resultBuffer.allocation, flags);
    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  BufferDma createBuffer(vk::DeviceSize                size_     = 0,
                         vk::BufferUsageFlags          usage_    = vk::BufferUsageFlags(),
                         const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return createBuffer({{}, size_, usage_}, memUsage_);
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  BufferDma createBuffer(const vk::CommandBuffer&      cmdBuf,
                         const vk::DeviceSize&         size_     = 0,
                         const void*                   data_     = nullptr,
                         const vk::BufferUsageFlags&   usage_    = vk::BufferUsageFlags(),
                         const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    // Create staging buffer with default priority
    float     oldPriority = m_allocator->setPriority();
    BufferDma stageBuffer = createBuffer(size_, vk::BufferUsageFlagBits::eTransferSrc,
                                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_stagingBuffers.push_back(stageBuffer);  // Remember the buffers to delete
    m_allocator->setPriority(oldPriority);

    // Copy the data to memory
    if(data_)
    {
      void* mapped = m_allocator->map(stageBuffer.allocation);
      memcpy(mapped, data_, size_);
      m_allocator->unmap(stageBuffer.allocation);
    }

    // Create the result buffer
    vk::BufferCreateInfo createInfoR{{}, size_, usage_ | vk::BufferUsageFlagBits::eTransferDst};
    BufferDma            resultBuffer = createBuffer(createInfoR, memUsage_);

    // Copy staging (need to submit command buffer, flushStaging must be done after submitting)
    cmdBuf.copyBuffer(stageBuffer.buffer, resultBuffer.buffer, vk::BufferCopy(0, 0, size_));

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  template <typename T>
  BufferDma createBuffer(const vk::CommandBuffer&      cmdBuff,
                         const std::vector<T>&         data_,
                         const vk::BufferUsageFlags&   usage_    = vk::BufferUsageFlags(),
                         const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return createBuffer(cmdBuff, sizeof(T) * data_.size(), data_.data(), usage_, memUsage_);
  }

  //--------------------------------------------------------------------------------------------------
  // Basic image creation
  ImageDma createImage(const vk::ImageCreateInfo& info_, const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    ImageDma                    resultImage;
    const VkMemoryPropertyFlags flags{memUsage_};
    resultImage.image = m_allocator->createImage(info_, resultImage.allocation, flags);
    return resultImage;
  }

  //--------------------------------------------------------------------------------------------------
  // Create an image with data
  //
  ImageDma createImage(const vk::CommandBuffer&      cmdBuff,
                       size_t                        size_,
                       const void*                   data_,
                       const vk::ImageCreateInfo&    info_,
                       const vk::ImageLayout&        layout_   = vk::ImageLayout::eShaderReadOnlyOptimal,
                       const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    ImageDma resultImage = createImage(info_, memUsage_);

    // Copy the data to staging buffer than to image
    if(data_ != nullptr)
    {
      // Create staging buffer with default priority
      float     oldPriority = m_allocator->setPriority();
      BufferDma stageBuffer = createBuffer(size_, vk::BufferUsageFlagBits::eTransferSrc,
                                           vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      m_stagingBuffers.push_back(stageBuffer);  // Remember the buffers to delete
      m_allocator->setPriority(oldPriority);

      // Copy data to buffer
      void* mapped = m_allocator->map(stageBuffer.allocation);
      memcpy(mapped, data_, size_);
      m_allocator->unmap(stageBuffer.allocation);

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
  AccelerationDma createAcceleration(vk::AccelerationStructureCreateInfoNV& accel_)
  {
    AccelerationDma resultAccel;

    // 1. Creating the acceleration structure
    auto accel = m_device.createAccelerationStructureNV(accel_);

    // 2. Finding memory to allocate
    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
    memoryRequirementsInfo.setAccelerationStructure(accel);
    const VkMemoryRequirements2 requirements = m_device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

    // 3. Allocate memory
    auto allocationID = m_allocator->alloc(requirements.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, nullptr);
    nvvk::Allocation allocation = allocationID.isValid() ? m_allocator->getAllocation(allocationID) : nvvk::Allocation();
    if(allocation.mem == VK_NULL_HANDLE)
    {
      m_device.destroy(accel);
      return resultAccel;
    }

    // 4. Bind memory to acceleration structure
    vk::BindAccelerationStructureMemoryInfoNV bind;
    bind.setAccelerationStructure(accel);
    bind.setMemory(allocation.mem);
    bind.setMemoryOffset(allocation.offset);
    assert(allocation.offset % requirements.memoryRequirements.alignment == 0);
    m_device.bindAccelerationStructureMemoryNV(bind);

    resultAccel.accel      = accel;
    resultAccel.allocation = allocationID;
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
  void destroy(BufferDma& b_)
  {
    m_device.destroyBuffer(b_.buffer);
    if(b_.allocation)
      m_allocator->free(b_.allocation);
  }

  void destroy(ImageDma& i_)
  {
    m_device.destroyImage(i_.image);
    if(i_.allocation)
      m_allocator->free(i_.allocation);
  }

  void destroy(TextureDma& t_)
  {
    m_device.destroyImageView(t_.descriptor.imageView);
    m_device.destroySampler(t_.descriptor.sampler);
    m_device.destroyImage(t_.image);
    if(t_.allocation)
      m_allocator->free(t_.allocation);
  }

  void destroy(AccelerationDma& a_)
  {
    m_device.destroyAccelerationStructureNV(a_.accel);
    if(a_.allocation)
      m_allocator->free(a_.allocation);
  }

  //--------------------------------------------------------------------------------------------------
  // Other
  //
  void* map(const BufferDma& buffer_) { return m_allocator->map(buffer_.allocation); }
  void  unmap(const BufferDma& buffer_) { m_allocator->unmap(buffer_.allocation); }


private:
  vk::Device                   m_device;
  nvvk::DeviceMemoryAllocator* m_allocator;
  std::vector<BufferDma>       m_stagingBuffers;

  struct GarbageCollection
  {
    vk::Fence              fence;
    std::vector<BufferDma> stagingBuffers;
  };
  std::vector<GarbageCollection> m_garbageBuffers;


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
          m_device.destroyBuffer(st.buffer);
          m_allocator->free(st.allocation);
        }
        s = m_garbageBuffers.erase(s);  // Done with it
      }
      else
      {
        ++s;
      }
    }
  }
};

}  // namespace nvvkpp
