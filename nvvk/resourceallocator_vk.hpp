/*
 * Copyright (c) 2019-2025, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

#include "memallocator_vk.hpp"
#include "samplers_vk.hpp"
#include "stagingmemorymanager_vk.hpp"
#include "sparse_image_vk.hpp"


/** @DOC_START
 # class nvvk::ResourceAllocator

 The goal of nvvk::ResourceAllocator is to aid creation of typical Vulkan
 resources (VkBuffer, VkImage and VkAccelerationStructure).
 All memory is allocated using the provided [nvvk::MemAllocator](#class-nvvkmemallocator)
 and bound to the appropriate resources. The allocator contains a 
 [nvvk::StagingMemoryManager](#class-nvvkstagingmemorymanager) and 
 [nvvk::SamplerPool](#class-nvvksamplerpool) to aid this process.

 ResourceAllocator separates object creation and memory allocation by delegating allocation 
 of memory to an object of interface type 'nvvk::MemAllocator'.
 This way the ResourceAllocator can be used with different memory allocation strategies, depending on needs.
 nvvk provides three implementations of MemAllocator:
 * nvvk::DedicatedMemoryAllocator is using a very simple allocation scheme, one VkDeviceMemory object per allocation.
   This strategy is only useful for very simple applications due to the overhead of vkAllocateMemory and 
   an implementation dependent bounded number of vkDeviceMemory allocations possible.
 * nvvk::DMAMemoryAllocator delegates memory requests to a 'nvvk:DeviceMemoryAllocator',
   as an example implemention of a suballocator. Not thread-safe.
 * nvvk::VMAMemoryAllocator delegates memory requests to a [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator), thread-safe
 
 Utility wrapper structs contain the appropriate Vulkan resource and the
 appropriate nvvk::MemHandle :

 - nvvk::Buffer
 - nvvk::Image
 - nvvk::Texture  contains VkImage and VkImageView as well as an 
   optional VkSampler stored witin VkDescriptorImageInfo
 - nvvk::AccelNV
 - nvvk::AccelKHR

 nvvk::Buffer, nvvk::Image, nvvk::Texture and nvvk::AccelKHR nvvk::AccelNV objects can be copied
 by value. They do not track lifetime of the underlying Vulkan objects and memory allocations. 
 The corresponding destroy() functions of nvvk::ResourceAllocator destroy created objects and
 free up their memory. ResourceAllocator does not track usage of objects either. Thus, one has to
 make sure that objects are no longer in use by the GPU when they get destroyed.

 > Note: These classes are foremost to showcase principle components that
 > a Vulkan engine would most likely have.
 > They are geared towards ease of use in this sample framework, and 
 > not optimized nor meant for production code.

 ```cpp
 VmaAllocator                vma;
 nvvk::VMAMemoryAllocator    memAllocator;
 nvvk::ResourceAllocator     resAllocator;

 vma .. init
 memAllocator.init(device, physicalDevice, vma);
 resAllocator.init(device, physicalDevice, &memAllocator);

 ...

 VkCommandBuffer cmd = ... transfer queue command buffer

 // creates new resources and 
 // implicitly triggers staging transfer copy operations into cmd
 nvvk::Buffer vbo = resAllocator.createBuffer(cmd, vboSize, vboData, vboUsage);
 nvvk::Buffer ibo = resAllocator.createBuffer(cmd, iboSize, iboData, iboUsage);

 // use functions from staging memory manager
 // here we associate the temporary staging resources with a fence
 resAllocator.finalizeStaging( fence );

 // submit cmd buffer with staging copy operations
 vkQueueSubmit(... cmd ... fence ...)

 ...

 // if you do async uploads you would
 // trigger garbage collection somewhere per frame
 resAllocator.releaseStaging();

 ```

 Separation of memory allocation and resource creation is very flexible, but it
 can be tedious to set up for simple usecases. nvvk offers three helper ResourceAllocator
 derived classes which internally contain the MemAllocator object and manage its lifetime:
 * [ResourceAllocatorDedicated](#class nvvk::ResourceAllocatorDedicated)
 * [ResourceAllocatorDma](#class nvvk::ResourceAllocatorDma)
 * [ResourceAllocatorVma](#cass nvvk::ResourceAllocatorVma)
 
 In these cases, only one object needs to be created and initialized. 
 
 ResourceAllocator can also be subclassed to specialize some of its functionality.
 Examples are [ExportResourceAllocator](#class ExportResourceAllocator) and [ExplicitDeviceMaskResourceAllocator](#class ExplicitDeviceMaskResourceAllocator).
 ExportResourceAllocator injects itself into the object allocation process such that 
 the resulting allocations can be exported or created objects may be bound to exported
 memory
 ExplicitDeviceMaskResourceAllocator overrides the devicemask of allocations such that
 objects can be created on a specific device in a device group.

 ## Multi-Threading

 For multi-threaded usage, for example dedicating a thread to handling uploads or downloads,
it is possible to use a ResourceAllocator per thread.

 ```
 ResourceAllocator resAllocatorPrimary;
 ResourceAllocator resAllocatorTransferThread;

 // make sure to use a thread-safe primary memory allocator
 resAllocator.init(...);

 // The res allocator used exclusively by the transfer thread will
 // use the primary's memory allocator and sampler pool. It will
 // get its own staging memory manager (which is not thread-safe)
 // so that staging memory can be accessed easily.
 resAllocatorTransferThread.init(resAllocatorPrimary);
 ```

 @DOC_END */

// Default size of the memory chunks used by large buffer allocations
#define NVVK_DEFAULT_LARGE_BUFFER_CHUNK_SIZE (2ull * 1024ull * 1024ull * 1024ull)

namespace nvvk {

// Objects
struct Buffer
{
  VkBuffer        buffer = VK_NULL_HANDLE;
  MemHandle       memHandle{};
  VkDeviceAddress address{0};
};

// Large buffers are used for buffers larger than the
// maximum size for a single allocation. Internally a large buffer
// is a sparse buffer where all pages are allocated.
// If the requested size is smaller than a single page, a single
// allocation is made and the behavior is the same as a nvvk::Buffer
struct LargeBuffer
{
  VkBuffer               buffer = VK_NULL_HANDLE;
  std::vector<MemHandle> memHandle{};
  VkDeviceAddress        address{0};
};


struct Image
{
  VkImage   image = VK_NULL_HANDLE;
  MemHandle memHandle{nullptr};
};

struct Texture
{
  VkImage               image = VK_NULL_HANDLE;
  MemHandle             memHandle{nullptr};
  VkDescriptorImageInfo descriptor{};
};

struct AccelNV
{
  VkAccelerationStructureNV accel = VK_NULL_HANDLE;
  MemHandle                 memHandle{nullptr};
};

struct AccelKHR
{
  VkAccelerationStructureKHR accel = VK_NULL_HANDLE;
  nvvk::Buffer               buffer;
  VkDeviceAddress            address{0};
};

struct LargeAccelKHR
{
  VkAccelerationStructureKHR accel = VK_NULL_HANDLE;
  nvvk::LargeBuffer          buffer;
  VkDeviceAddress            address{0};
};

//--------------------------------------------------------------------------------------------------
// Allocator for buffers, images and acceleration structures
//
class StagingMemoryManager;


class ResourceAllocator
{
public:
  ResourceAllocator(ResourceAllocator const&)            = delete;
  ResourceAllocator& operator=(ResourceAllocator const&) = delete;

  ResourceAllocator() = default;

  // Both `memAllocator`, and the optional `externalSamplerPool` are directly referenced,
  // no ownership transfer.
  ResourceAllocator(VkDevice           device,
                    VkPhysicalDevice   physicalDevice,
                    MemAllocator*      memAllocator,
                    VkDeviceSize       stagingBlockSize             = NVVK_DEFAULT_STAGING_BLOCKSIZE,
                    VkBufferUsageFlags stagingExtraBufferUsageFlags = 0,
                    SamplerPool*       externalSamplerPool          = nullptr);

  ResourceAllocator(ResourceAllocator& other);

  // All staging buffers must be cleared before
  virtual ~ResourceAllocator();

  //--------------------------------------------------------------------------------------------------
  // Initialization of the allocator
  // Both `memAllocator`, and the optional `externalSamplerPool` are directly referenced,
  // no ownership transfer.
  void init(VkDevice           device,
            VkPhysicalDevice   physicalDevice,
            MemAllocator*      memAlloc,
            VkDeviceSize       stagingBlockSize             = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkBufferUsageFlags stagingExtraBufferUsageFlags = 0,
            SamplerPool*       externalSamplerPool          = nullptr);

  // inherits memAllocator and samplerPool from other
  void init(ResourceAllocator& other);

  void deinit();

  MemAllocator* getMemoryAllocator() { return m_memAlloc; }

  SamplerPool* getSamplerPool() { return m_samplerPool; }

  // can return nullptr if the sampler pool is not owned by this resource allocator
  SamplerPool* getInternalSamplerPool() { return m_samplerPoolInternal.get(); }

  //--------------------------------------------------------------------------------------------------
  // Basic buffer creation
  nvvk::Buffer createBuffer(const VkBufferCreateInfo& info_, const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
  nvvk::Buffer createBuffer(VkDeviceSize                size_     = 0,
                            VkBufferUsageFlags          usage_    = VkBufferUsageFlags(),
                            const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation with data uploaded through staging manager
  // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
  nvvk::Buffer createBuffer(const VkCommandBuffer& cmdBuf,
                            const VkDeviceSize&    size_,
                            const void*            data_,
                            VkBufferUsageFlags     usage_,
                            VkMemoryPropertyFlags  memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation with data uploaded through staging manager
  // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
  template <typename T>
  nvvk::Buffer createBuffer(const VkCommandBuffer& cmdBuf,
                            const std::vector<T>&  data_,
                            VkBufferUsageFlags     usage_,
                            VkMemoryPropertyFlags  memProps_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    return createBuffer(cmdBuf, sizeof(T) * data_.size(), data_.data(), usage_, memProps_);
  }


  //--------------------------------------------------------------------------------------------------
  // Basic large buffer creation
  // If the requested size is < maxChunkSize, a single allocation is made and the behavior is the same as a nvvk::Buffer
  nvvk::LargeBuffer createLargeBuffer(VkQueue                     queue,
                                      const VkBufferCreateInfo&   info_,
                                      const VkMemoryPropertyFlags memProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      VkDeviceSize                maxChunkSize = NVVK_DEFAULT_LARGE_BUFFER_CHUNK_SIZE);

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation
  // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
  // If the requested size is < maxChunkSize, a single allocation is made and the behavior is the same as a nvvk::Buffer
  nvvk::LargeBuffer createLargeBuffer(VkQueue                     queue,
                                      VkDeviceSize                size_,
                                      VkBufferUsageFlags          usage_,
                                      const VkMemoryPropertyFlags memUsage_    = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      VkDeviceSize                maxChunkSize = NVVK_DEFAULT_LARGE_BUFFER_CHUNK_SIZE);

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation with data uploaded through staging manager
  // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
  // If the requested size is < maxChunkSize, a single allocation is made and the behavior is the same as a nvvk::Buffer
  nvvk::LargeBuffer createLargeBuffer(VkQueue                queue,
                                      const VkCommandBuffer& cmdBuf,
                                      const VkDeviceSize&    size_,
                                      const void*            data_,
                                      VkBufferUsageFlags     usage_,
                                      VkMemoryPropertyFlags  memProps     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      VkDeviceSize           maxChunkSize = NVVK_DEFAULT_LARGE_BUFFER_CHUNK_SIZE);

  //--------------------------------------------------------------------------------------------------
  // Simple buffer creation with data uploaded through staging manager
  // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
  // If the requested size is < maxChunkSize, a single allocation is made and the behavior is the same as a nvvk::Buffer
  template <typename T>
  nvvk::LargeBuffer createLargeBuffer(VkQueue                queue,
                                      const VkCommandBuffer& cmdBuf,
                                      const std::vector<T>&  data_,
                                      VkBufferUsageFlags     usage_,
                                      VkMemoryPropertyFlags  memProps_    = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      VkDeviceSize           maxChunkSize = NVVK_DEFAULT_LARGE_BUFFER_CHUNK_SIZE)
  {
    return createLargeBuffer(queue, cmdBuf, sizeof(T) * data_.size(), data_.data(), usage_, memProps_);
  }


  //--------------------------------------------------------------------------------------------------
  // Basic image creation
  nvvk::Image createImage(const VkImageCreateInfo& info_, const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


  //--------------------------------------------------------------------------------------------------
  // Create an image with data uploaded through staging manager
  nvvk::Image createImage(const VkCommandBuffer&   cmdBuf,
                          size_t                   size_,
                          const void*              data_,
                          const VkImageCreateInfo& info_,
                          const VkImageLayout&     layout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  //--------------------------------------------------------------------------------------------------
  // other variants could exist with a few defaults but we already have nvvk::makeImage2DViewCreateInfo()
  // we could always override viewCreateInfo.image
  nvvk::Texture createTexture(const Image& image, const VkImageViewCreateInfo& imageViewCreateInfo);
  nvvk::Texture createTexture(const Image& image, const VkImageViewCreateInfo& imageViewCreateInfo, const VkSamplerCreateInfo& samplerCreateInfo);

  //--------------------------------------------------------------------------------------------------
  // shortcut that creates the image for the texture
  // - creates the image
  // - creates the texture part by associating image and sampler
  //
  nvvk::Texture createTexture(const VkCommandBuffer&     cmdBuf,
                              size_t                     size_,
                              const void*                data_,
                              const VkImageCreateInfo&   info_,
                              const VkSamplerCreateInfo& samplerCreateInfo,
                              const VkImageLayout&       layout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              bool                       isCube  = false);

  nvvk::SparseImage createSparseImage(VkImageCreateInfo           info_,
                                      const VkMemoryPropertyFlags memUsage_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


  void flushSparseImage(SparseImage& sparseImage);

  bool createSparseImagePage(SparseImage& sparseImage, uint32_t pageIndex, uint32_t layer = 0u);

  //--------------------------------------------------------------------------------------------------
  // Create the acceleration structure
  //
  nvvk::AccelNV createAcceleration(const VkAccelerationStructureCreateInfoNV& accel_);


  //--------------------------------------------------------------------------------------------------
  // Create the acceleration structure
  //
  nvvk::AccelKHR createAcceleration(const VkAccelerationStructureCreateInfoKHR& accel_);


  //--------------------------------------------------------------------------------------------------
  // Create the acceleration structure with LargeBuffer as underlying storage
  // This allows for very large acceleration structures > 4GB; utilizing sparse buffer bindings underneath
  // Provide a queue to perform the vkQueueSparseBinding() call on
  LargeAccelKHR createLargeAcceleration(VkQueue queue, const VkAccelerationStructureCreateInfoKHR& accel_);


  //--------------------------------------------------------------------------------------------------
  // Acquire a sampler with the provided information (see nvvk::SamplerPool for details).
  // Every acquire must have an appropriate release for appropriate internal reference counting
  VkSampler acquireSampler(const VkSamplerCreateInfo& info);
  void      releaseSampler(VkSampler sampler);

  //--------------------------------------------------------------------------------------------------
  // implicit staging operations triggered by create are managed here
  void finalizeStaging(VkFence fence = VK_NULL_HANDLE);
  void finalizeAndReleaseStaging(VkFence fence = VK_NULL_HANDLE);
  void releaseStaging();

  StagingMemoryManager*       getStaging();
  const StagingMemoryManager* getStaging() const;


  //--------------------------------------------------------------------------------------------------
  // Destroy
  //
  void destroy(nvvk::Buffer& b_);
  void destroy(nvvk::LargeBuffer& b_);
  void destroy(nvvk::Image& i_);
  void destroy(nvvk::AccelNV& a_);
  void destroy(nvvk::AccelKHR& a_);
  void destroy(nvvk::Texture& t_);
  void destroy(nvvk::SparseImage& i_);
  // Destroy a sparse image page. Returns true if that page actually was present in memory
  bool destroy(nvvk::SparseImage& i_, uint32_t pageIndex, uint32_t layer = 0);

  //--------------------------------------------------------------------------------------------------
  // Other
  //
  void* map(const nvvk::Buffer& buffer);
  void  unmap(const nvvk::Buffer& buffer);
  void* map(const nvvk::Image& image);
  void  unmap(const nvvk::Image& image);

  VkDevice         getDevice() const { return m_device; }
  VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }


  void destroy(LargeAccelKHR& a_);


protected:
  // If necessary, these can be overriden to specialize the allocation, for instance to
  // enforce allocation of exportable
  virtual MemHandle AllocateMemory(const MemAllocateInfo& allocateInfo);
  virtual void      CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer);
  virtual void      CreateImageEx(const VkImageCreateInfo& info_, VkImage* image);

  //--------------------------------------------------------------------------------------------------
  // Finding the memory type for memory allocation
  //
  uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties);

  VkDevice                              m_device{VK_NULL_HANDLE};
  VkPhysicalDevice                      m_physicalDevice{VK_NULL_HANDLE};
  VkPhysicalDeviceMemoryProperties      m_memoryProperties{};
  MemAllocator*                         m_memAlloc{nullptr};
  VkBufferUsageFlags                    m_stagingExtraBufferUsageFlags = 0;
  std::unique_ptr<StagingMemoryManager> m_staging;
  std::unique_ptr<SamplerPool>          m_samplerPoolInternal;
  SamplerPool*                          m_samplerPool;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DeviceMemoryAllocator;

/** @DOC_START
 # class nvvk::ResourceAllocatorDma
 nvvk::ResourceAllocatorDMA is a convencience class owning a nvvk::DeviceMemoryAllocator object.
 **Not** thread-safe.
@DOC_END */
class ResourceAllocatorDma : public ResourceAllocator
{
public:
  ResourceAllocatorDma() = default;
  ResourceAllocatorDma(VkDevice         device,
                       VkPhysicalDevice physicalDevice,
                       VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
                       VkDeviceSize     memBlockSize     = 0);
  virtual ~ResourceAllocatorDma();

  void init(VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkDeviceSize     memBlockSize     = 0);
  // Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
  void init(VkInstance,
            VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkDeviceSize     memBlockSize     = 0);

  void deinit();

  nvvk::DeviceMemoryAllocator*       getDMA() { return m_dma; }
  const nvvk::DeviceMemoryAllocator* getDMA() const { return m_dma; }

protected:
  nvvk::DeviceMemoryAllocator* m_dma = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class DMAMemoryAllocatorTS;

/** @DOC_START
# class nvvk::ResourceAllocatorDma
nvvk::ResourceAllocatorDMA is a convencience class owning a nvvk::DeviceMemoryAllocator and its simple thread-safe wrapper nvvk::DMAMemoryAllocatorTS object.
@DOC_END */
class ResourceAllocatorDmaTS : public ResourceAllocator
{
public:
  ResourceAllocatorDmaTS() = default;
  ResourceAllocatorDmaTS(VkDevice         device,
                         VkPhysicalDevice physicalDevice,
                         VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
                         VkDeviceSize     memBlockSize     = 0);
  virtual ~ResourceAllocatorDmaTS();

  void init(VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkDeviceSize     memBlockSize     = 0);
  // Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
  void init(VkInstance,
            VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkDeviceSize     memBlockSize     = 0);

  void deinit();

  nvvk::DMAMemoryAllocatorTS*       getDMA() { return m_dmaTS; }
  const nvvk::DMAMemoryAllocatorTS* getDMA() const { return m_dmaTS; }

protected:
  nvvk::DeviceMemoryAllocator* m_dma   = nullptr;
  nvvk::DMAMemoryAllocatorTS*  m_dmaTS = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @DOC_START
 # class nvvk::ResourceAllocatorDedicated
 >  nvvk::ResourceAllocatorDedicated is a convencience class automatically creating and owning a DedicatedMemoryAllocator object
 @DOC_END */
class ResourceAllocatorDedicated : public ResourceAllocator
{
public:
  ResourceAllocatorDedicated() = default;
  ResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  virtual ~ResourceAllocatorDedicated();

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  // Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
  void init(VkInstance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

  void deinit();

protected:
  std::unique_ptr<MemAllocator> m_memAlloc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @DOC_START
 #class nvvk::ExportResourceAllocator

 ExportResourceAllocator specializes the object allocation process such that resulting memory allocations are
 exportable and buffers and images can be bound to external memory.
@DOC_END */
class ExportResourceAllocator : public ResourceAllocator
{
public:
  ExportResourceAllocator() = default;
  ExportResourceAllocator(VkDevice         device,
                          VkPhysicalDevice physicalDevice,
                          MemAllocator*    memAlloc,
                          VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

protected:
  virtual MemHandle AllocateMemory(const MemAllocateInfo& allocateInfo) override;
  virtual void      CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer) override;
  virtual void      CreateImageEx(const VkImageCreateInfo& info_, VkImage* image) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @DOC_START
 # class nvvk::ExportResourceAllocatorDedicated
 nvvk::ExportResourceAllocatorDedicated is a resource allocator that is using DedicatedMemoryAllocator to allocate memory
 and at the same time it'll make all allocations exportable.
@DOC_END */
class ExportResourceAllocatorDedicated : public ExportResourceAllocator
{
public:
  ExportResourceAllocatorDedicated() = default;
  ExportResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  virtual ~ExportResourceAllocatorDedicated() override;

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  void deinit();

protected:
  std::unique_ptr<MemAllocator> m_memAlloc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @DOC_START
 # class nvvk::ExplicitDeviceMaskResourceAllocator
 nvvk::ExplicitDeviceMaskResourceAllocator is a resource allocator that will inject a specific devicemask into each
 allocation, making the created allocations and objects available to only the devices in the mask.
@DOC_END */
class ExplicitDeviceMaskResourceAllocator : public ResourceAllocator
{
public:
  ExplicitDeviceMaskResourceAllocator() = default;
  ExplicitDeviceMaskResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, uint32_t deviceMask);

  void init(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, uint32_t deviceMask);

protected:
  virtual MemHandle AllocateMemory(const MemAllocateInfo& allocateInfo) override;

  uint32_t m_deviceMask;
};

}  // namespace nvvk
