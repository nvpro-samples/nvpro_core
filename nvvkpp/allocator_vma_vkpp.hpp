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
#include "utilities_vkpp.hpp"
#include <basetsd.h>
#include <iostream>
#include <vk_mem_alloc.h>

//--------------------------------------------------------------------------------------------------
// Buffer, image and acceleration structure allocator using: Vulkan Memory Allocator
// See: // https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
// Helps creating resources and keeping the allocation information in simple structure
//


namespace nvvkpp {

// Objects
struct BufferVma
{
  vk::Buffer    buffer;
  VmaAllocation allocation{};
};

struct ImageVma
{
  vk::Image     image;
  VmaAllocation allocation{};
};

struct TextureVma : public ImageVma
{
  vk::DescriptorImageInfo descriptor;

  TextureVma& operator=(const ImageVma& buffer)
  {
    static_cast<ImageVma&>(*this) = buffer;
    return *this;
  }
};

struct AccelerationVma
{
  vk::AccelerationStructureNV accel;
  VmaAllocation               allocation{};
};

//--------------------------------------------------------------------------------------------------
// Allocator for buffers, images and acceleration structure using Vulkan Memory Allocator
// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//
class AllocatorVma
{
public:
  // All staging buffers must be cleared before
  ~AllocatorVma() { assert(m_stagingBuffers.empty()); }

  //--------------------------------------------------------------------------------------------------
  // Initialization of the allocator
  void init(vk::Device device, VmaAllocator allocator)
  {
    m_device    = device;
    m_allocator = allocator;
  }

  //--------------------------------------------------------------------------------------------------
  // Basic buffer creation
  BufferVma createBuffer(const vk::BufferCreateInfo& info_, VmaMemoryUsage memUsage_ = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    BufferVma                resultBuffer;
    const VkBufferCreateInfo sInfo = info_;
    VmaAllocationCreateInfo  allocInfo{};
    allocInfo.usage = memUsage_;
    VkBuffer       tmpBuffer;
    const VkResult result = vmaCreateBuffer(m_allocator, &sInfo, &allocInfo, &tmpBuffer, &resultBuffer.allocation, nullptr);
    assert(result == VK_SUCCESS);
    resultBuffer.buffer = tmpBuffer;
    //    assert(PtrToInt(tmpBuffer) != 0xa0);
    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  BufferVma createBuffer(vk::DeviceSize size_ = 0, vk::BufferUsageFlags usage_ = vk::BufferUsageFlags(), VmaMemoryUsage memUsage_ = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    return createBuffer({{}, size_, usage_}, memUsage_);
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  BufferVma createBuffer(const vk::CommandBuffer&    cmdBuf,
                         const vk::DeviceSize&       size_  = 0,
                         const void*                 data_  = nullptr,
                         const vk::BufferUsageFlags& usage_ = vk::BufferUsageFlags())
  {
    // Create staging buffer
    BufferVma stageBuffer = createBuffer(size_, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
    m_stagingBuffers.push_back(stageBuffer);  // Remember the buffers to delete

    // Copy the data to memory
    if(data_)
    {
      void* mapped;
      vmaMapMemory(m_allocator, stageBuffer.allocation, &mapped);
      memcpy(mapped, data_, size_);
      vmaUnmapMemory(m_allocator, stageBuffer.allocation);
    }

    // Create the result buffer
    const vk::BufferCreateInfo createInfoR{{}, size_, usage_ | vk::BufferUsageFlagBits::eTransferDst};
    BufferVma                  resultBuffer = createBuffer(createInfoR);

    // Copy staging (need to submit command buffer, flushStaging must be done after submitting)
    cmdBuf.copyBuffer(stageBuffer.buffer, resultBuffer.buffer, vk::BufferCopy(0, 0, size_));

    return resultBuffer;
  }

  //--------------------------------------------------------------------------------------------------
  // Staging buffer creation, uploading data to device buffer
  template <typename T>
  BufferVma createBuffer(const vk::CommandBuffer&    cmdBuff,
                         const std::vector<T>&       data_,
                         const vk::BufferUsageFlags& usage_ = vk::BufferUsageFlags())
  {
    return createBuffer(cmdBuff, sizeof(T) * data_.size(), data_.data(), usage_);
  }

  //--------------------------------------------------------------------------------------------------
  // Basic image creation
  ImageVma createImage(const vk::ImageCreateInfo& info_, VmaMemoryUsage memUsage_ = VMA_MEMORY_USAGE_GPU_ONLY)
  {
    ImageVma                resultImage;
    const VkImageCreateInfo cInfo     = info_;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = memUsage_;
    VkImage tmpImage;
    vmaCreateImage(m_allocator, &cInfo, &allocInfo, &tmpImage, &resultImage.allocation, nullptr);
    resultImage.image = tmpImage;
    return resultImage;
  }

  //--------------------------------------------------------------------------------------------------
  // Create an image with data
  //
  ImageVma createImage(const vk::CommandBuffer&   cmdBuff,
                       size_t                     size_,
                       const void*                data_,
                       const vk::ImageCreateInfo& info_,
                       const vk::ImageLayout&     layout_ = vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    ImageVma resultImage = createImage(info_);

    // Copy the data to staging buffer than to image
    if(data_ != nullptr)
    {
      BufferVma stageBuffer = createBuffer(size_, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
      m_stagingBuffers.push_back(stageBuffer);  // Remember the buffers to delete

      // Copy data to buffer
      void* mapped;
      vmaMapMemory(m_allocator, stageBuffer.allocation, &mapped);
      memcpy(mapped, data_, size_);
      vmaUnmapMemory(m_allocator, stageBuffer.allocation);

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
  AccelerationVma createAcceleration(const vk::AccelerationStructureCreateInfoNV& accel_)
  {
    AccelerationVma resultAccel;
    // 1. Create the acceleration structure
    resultAccel.accel = m_device.createAccelerationStructureNV(accel_);

    // 2. Find the memory requirements
    vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
    memoryRequirementsInfo.setAccelerationStructure(resultAccel.accel);
    const VkMemoryRequirements2 requirements = m_device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);

    // 3. Allocate memory using allocator.
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocateMemory(m_allocator, &requirements.memoryRequirements, &allocCreateInfo, &resultAccel.allocation, nullptr);

    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(m_allocator, resultAccel.allocation, &allocInfo);
    assert(allocInfo.offset % requirements.memoryRequirements.alignment == 0);

    // 4. Bind Acceleration struct and buffer
    vk::BindAccelerationStructureMemoryInfoNV bind;
    bind.setAccelerationStructure(resultAccel.accel);
    bind.setMemory(allocInfo.deviceMemory);
    bind.setMemoryOffset(allocInfo.offset);

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
  void destroy(BufferVma& b_)
  {
    m_device.destroyBuffer(b_.buffer);
    vmaFreeMemory(m_allocator, b_.allocation);
  }

  void destroy(ImageVma& i_)
  {
    m_device.destroyImage(i_.image);
    vmaFreeMemory(m_allocator, i_.allocation);
  }

  void destroy(AccelerationVma& a_)
  {
    m_device.destroyAccelerationStructureNV(a_.accel);  //, nullptr, NVVKPP_DISPATCHER);
    vmaFreeMemory(m_allocator, a_.allocation);
  }

  void destroy(TextureVma& t_)
  {
    m_device.destroyImageView(t_.descriptor.imageView);
    m_device.destroySampler(t_.descriptor.sampler);
    destroy(static_cast<ImageVma&>(t_));
  }

  //--------------------------------------------------------------------------------------------------
  // Other
  //
  void* map(const BufferVma& buffer_)
  {
    void* mapped;
    vmaMapMemory(m_allocator, buffer_.allocation, &mapped);
    return mapped;
  }
  void unmap(const BufferVma& buffer_) { vmaUnmapMemory(m_allocator, buffer_.allocation); }


private:
  vk::Device             m_device;
  VmaAllocator           m_allocator{};
  std::vector<BufferVma> m_stagingBuffers;

  struct GarbageCollection
  {
    vk::Fence              fence;
    std::vector<BufferVma> stagingBuffers;
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
          vmaFreeMemory(m_allocator, st.allocation);
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

/////////////////////////////////////////////////////////////////////////
/**
# Allocators in nvvkpp
Memory allocation shouldn't be a one-to-one with buffers and images, but larger memory block should 
be allocated and the buffers and images are mapped to a section of it. For better management of 
memory, it is suggested to use [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator).  
But in some cases, like for Vulkan Interop, the best is to use `AllocatorVkExport` which is export 
all memory allocations and make them available for Cuda or OpenGL.

## Initialization
For VMA, you need first to create the `VmaAllocator`. In the following example, it creates the allocator and also will use dedicated memory in some cases.
~~~~C++
    VmaAllocator m_vmaAllocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = physicalDevice;
    allocatorInfo.device                 = device;
    allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                         VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
    vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
~~~~

> Note: For dedicated memory, it is required to enable device extensions: <br>`VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME` and `VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME`

Then initialize the allocator itself:
~~~~c++
    nvvkpp::AllocatorVma m_alloc;
    m_alloc.init(device, m_vmaAllocator);
~~~~



## Buffers
Either you create a simple buffer using `createBuffer(bufferCreateInfo)` which is mostly for allocating buffer on the device or you upload data using `createBuffer(cmdBuff, size, data)`. 
The second one will be staging the transfer of the data to the device and there is a version where you can pass a std::vector instead of size and data.

Example
~~~~C++
  // Creating a device buffer
  m_sceneBuffer = m_alloc.createBuffer({}, sizeof(SceneUBO), vk::BufferUsageFlagBits::eUniformBuffer |
                                                             vk::BufferUsageFlagBits::eTransferDst});

  // Creating a device buffer and uploading the vertices
  m_vertexBuffer = m_alloc.createBuffer(cmdBuf, m_vertices.position,
                                        vk::BufferUsageFlagBits::eVertexBuffer |
                                        vk::BufferUsageFlagBits::eStorageBuffer);
~~~~

The Vk, DMA or VMA createBuffer are slightly different. They all have similar input and all return **BufferVk**, but the associated memory in the structure varies.

Example of the buffer structure for VMA
~~~~C++
struct BufferVma
{
  vk::Buffer    buffer;
  VmaAllocation allocation{};
};
~~~~



## Images
For images, it is identical to buffers. Either you create only an image, or you create and initialize it with data.
See the usage [example](#full-example) below.

Image structure for VMA:
~~~~C++
struct ImageVma
{
  vk::Image     image;
  VmaAllocation allocation{};
};
~~~~

## Textures
For convenience, there is also a texture structure that differs from the image by the addition of 
the descriptor which has the sampler and image view required for be used in shaders.

~~~~C++
struct TextureVma : public ImageVma
{
  vk::DescriptorImageInfo descriptor;
}
~~~~

To help creating textures and images this there is a few functions in `images_vkpp`:
* `create2DInfo`: return `vk::ImageCreateInfo`, which is used for the creation of the image
* `create2DDescriptor`: returns the `vk::ImageDescritorInfo`
* `generateMipmaps`: to generate all mipmaps level of an image
* `setImageLayout`: transition the image layout (`vk::ImageLayout`)


## Full example
~~~~C++
vk::Format          format = vk::Format::eR8G8B8A8Unorm;
vk::Extent2D        imgSize{width, height};
vk::ImageCreateInfo imageCreateInfo = nvvkpp::image::create2DInfo(imgSize, format, vk::ImageUsageFlagBits::eSampled, true);
vk::DeviceSize      bufferSize = width * height * 4 * sizeof(uint8_t);

m_texture = m_alloc.createImage(cmdBuf, bufferSize, buffer, imageCreateInfo);

vk::SamplerCreateInfo samplerCreateInfo{{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
nvvkpp::image::generateMipmaps(cmdBuf, m_texture.image, format, imgSize, imageCreateInfo.mipLevels);
m_texture.descriptor = nvvkpp::image::create2DDescriptor(m_device, m_texture.image, samplerCreateInfo);
~~~~




## Acceleration structure
For this one, there are no staging variant, it will return the acceleration structure and memory binded.

## Destroy
To destroy buffers, image, or acceleration structures call the `destroy()` method with the object in argument. It will destroy the Vulkkan object and free the memory.

~~~~C++
  m_alloc.destroy(m_vertexBuffer);
  m_alloc.destroy(m_texture);
~~~~

---------------------------------------------

# Staging
In case data was uploaded using one of the staging method, it will be important to _flush_ the temporary allocations. You can call `flushStaging` directly after submitting the command buffer or pass a `vk::Fence` corresponding to the command buffer submission. See next section on MultipleCommandBuffers.

Flushing is required to recover memory, but cannot be done until the copy is completed. This is why there is an argument to pass a fence. Either you are making sure the Queue on which the command buffer that have placed the copy commands is idle, or the internal system will flush the staging buffers when the fence is released.

**Method 1 - Good**
~~~~C++
  // Submit current command buffer
  auto fence = m_cmdBufs.submit();
  // Adds all staging buffers to garbage collection, delete what it can
  m_alloc.flushStaging(fence);
~~~~

**Method 2 - Not so Good**
~~~~C++
m_cmdBufs.submit();        // Submit current command buffer
m_cmdBufs.waitForUpload(); // Wait that all commands are done
m_alloc.flushStaging();    // Destroy staging buffers
~~~~


*/
}  // namespace nvvkpp
