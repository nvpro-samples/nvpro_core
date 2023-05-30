/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <cassert>
#include <vector>
#include <string>

#include <nvvk/memallocator_vk.hpp>
#include <nvh/trangeallocator.hpp>

#include <vulkan/vulkan_core.h>

namespace nvvk {

#define NVVK_DEFAULT_MEMORY_BLOCKSIZE (VkDeviceSize(128) * 1024 * 1024)



//////////////////////////////////////////////////////////////////////////
/**
  This framework assumes that memory heaps exists that support:

  - VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    for uploading data to the device
  - VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_CACHED_BIT
    for downloading data from the device

  This is typical on all major desktop platforms and vendors.
  See http://vulkan.gpuinfo.org for information of various devices and platforms.

  # functions in nvvk

  * getMemoryInfo : fills the VkMemoryAllocateInfo based on device's memory properties and memory requirements and property flags. Returns `true` on success.
*/

// returns true on success
bool getMemoryInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
                   const VkMemoryRequirements&             memReqs,
                   VkMemoryPropertyFlags                   properties,
                   VkMemoryAllocateInfo&                   memInfo,
                   bool preferDevice = true);  // special case if zero properties are unsupported, otherwise use host

//////////////////////////////////////////////////////////////////////////

static const uint32_t INVALID_ID_INDEX = ~0;

struct Allocation
{
  VkDeviceMemory mem    = VK_NULL_HANDLE;
  VkDeviceSize   offset = 0;
  VkDeviceSize   size   = 0;
};

class AllocationID
{
  friend class DeviceMemoryAllocator;

private:
  uint32_t index      = INVALID_ID_INDEX;
  uint32_t generation = 0;

  void     invalidate() { index = INVALID_ID_INDEX; }
  uint32_t instantiate(uint32_t newIndex)
  {
    uint32_t oldIndex = index;
    index             = newIndex;
    generation++;

    return oldIndex;
  }

public:
  bool isValid() const { return index != INVALID_ID_INDEX; }
  bool isEqual(const AllocationID& other) const { return index == other.index && generation == other.generation; }

  operator bool() const { return isValid(); }

  friend bool operator==(const AllocationID& lhs, const AllocationID& rhs) { return rhs.isEqual(lhs); }
};


//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::DeviceMemoryAllocator

  The nvvk::DeviceMemoryAllocator allocates and manages device memory in fixed-size memory blocks.
  It implements the nvvk::MemAllocator interface.

  It sub-allocates from the blocks, and can re-use memory if it finds empty
  regions. Because of the fixed-block usage, you can directly create resources
  and don't need a phase to compute the allocation sizes first.

  It will create compatible chunks according to the memory requirements and
  usage flags. Therefore you can easily create mappable host allocations
  and delete them after usage, without inferring device-side allocations.

  An `AllocationID` is returned rather than the allocation details directly, which
  one can query separately.

  Several utility functions are provided to handle the binding of memory
  directly with the resource creation of buffers, images and acceleration
  structures. These utilities also make implicit use of Vulkan's dedicated
  allocation mechanism.

  We recommend the use of the nvvk::ResourceAllocator class, 
  rather than the various create functions provided here, as we may deprecate them.

  > **WARNING** : The memory manager serves as proof of concept for some key concepts
  > however it is not meant for production use and it currently lacks de-fragmentation logic
  > as well. You may want to look at [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
  > for a more production-focused solution.

  You can derive from this class and overload a few functions to alter the
  chunk allocation behavior.

  Example :
  \code{.cpp}
  nvvk::DeviceMemoryAllocator memAllocator;

  memAllocator.init(device, physicalDevice);

  // low-level
  aid = memAllocator.alloc(memRequirements,...);
  ...
  memAllocator.free(aid);

  // utility wrapper
  buffer = memAllocator.createBuffer(bufferSize, bufferUsage, bufferAid);
  ...
  memAllocator.free(bufferAid);


  // It is also possible to not track individual resources
  // and free everything in one go. However, this is
  // not recommended for general purpose use.

  bufferA = memAllocator.createBuffer(sizeA, usageA);
  bufferB = memAllocator.createBuffer(sizeB, usageB);
  ...
  memAllocator.freeAll();

  \endcode
*/
class DeviceMemoryAllocator : public MemAllocator
{

public:
  static const float DEFAULT_PRIORITY;

  DeviceMemoryAllocator(DeviceMemoryAllocator const&) = delete;
  DeviceMemoryAllocator& operator=(DeviceMemoryAllocator const&) = delete;


  virtual ~DeviceMemoryAllocator()
  {
#ifndef NDEBUG
    // If all memory was released properly, no blocks should be alive at this point
    assert(m_blocks.empty() || m_keepFirst);
#endif
    deinit();
  }


  // system related

  DeviceMemoryAllocator() { m_debugName = "nvvk::DeviceMemoryAllocator:" + std::to_string((uint64_t)this); }
  DeviceMemoryAllocator(VkDevice         device,
                        VkPhysicalDevice physicalDevice,
                        VkDeviceSize     blockSize = NVVK_DEFAULT_MEMORY_BLOCKSIZE,
                        VkDeviceSize     maxSize   = 0)
  {
    init(device, physicalDevice, blockSize, maxSize);
  }

  void init(VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     blockSize = NVVK_DEFAULT_MEMORY_BLOCKSIZE,
            VkDeviceSize     maxSize   = 0);

  void setDebugName(const std::string& name) { m_debugName = name; }

  // requires VK_EXT_memory_priority, default is false
  void setPrioritySupported(bool state) { m_supportsPriority = state; }

  // frees all blocks independent of individual allocations
  // use only if you know the lifetime of all resources from this allocator.
  void freeAll();

  // asserts on all resources being freed properly
  void deinit();

  // get utilization of block allocations
  float getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const;
  // get total amount of active blocks / VkDeviceMemory allocations
  uint32_t getActiveBlockCount() const { return m_activeBlockCount; }

  // dump detailed stats via nvprintfLevel(LOGLEVEL_INFO
  void nvprintReport() const;

  void getTypeStats(uint32_t     count[VK_MAX_MEMORY_TYPES],
                    VkDeviceSize used[VK_MAX_MEMORY_TYPES],
                    VkDeviceSize allocated[VK_MAX_MEMORY_TYPES]) const;

  const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;
  VkDeviceSize                            getMaxAllocationSize() const;

  //////////////////////////////////////////////////////////////////////////
  
  // Implement MemAllocator interface
  virtual MemHandle allocMemory(const MemAllocateInfo& allocInfo, VkResult *pResult = nullptr) override;
  virtual void      freeMemory(MemHandle memHandle) override;
  virtual MemInfo   getMemoryInfo(MemHandle memHandle) const override;
  virtual void*     map(MemHandle memHandle, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE, VkResult *pResult = nullptr) override;
  virtual void      unmap(MemHandle memHandle) override;

  virtual VkDevice         getDevice() const override;
  virtual VkPhysicalDevice getPhysicalDevice() const override;

  AllocationID      getAllocationID(MemHandle memHandle) const;

  //////////////////////////////////////////////////////////////////////////

  struct State
  {
    float                 priority           = DEFAULT_PRIORITY;
    VkMemoryAllocateFlags allocateFlags      = 0;
    uint32_t              allocateDeviceMask = 0;
  };

  // subsequent allocations (and creates) will use the provided priority
  // ignored if setPrioritySupported is not enabled
  float setPriority(float priority = DEFAULT_PRIORITY)
  {
    float old  = m_defaultState.priority;
    m_defaultState.priority = priority;
    return old;
  }

  float getPriority() const { return m_defaultState.priority; }

  // subsequent allocations (and creates) will use the provided flags
  void setAllocateFlags(VkMemoryAllocateFlags flags, bool enabled)
  {
    if(enabled)
    {
      m_defaultState.allocateFlags |= flags;
    }
    else
    {
      m_defaultState.allocateFlags &= ~flags;
    }
  }

  void setAllocateDeviceMask(uint32_t allocateDeviceMask, bool enabled)
  {
    if(enabled)
    {
      m_defaultState.allocateDeviceMask |= allocateDeviceMask;
    }
    else
    {
      m_defaultState.allocateDeviceMask &= ~allocateDeviceMask;
    }
  }

  VkMemoryAllocateFlags getAllocateFlags()      const { return m_defaultState.allocateFlags; }
  uint32_t              getAllocateDeviceMask() const { return m_defaultState.allocateDeviceMask; }

  // make individual raw allocations.
  // there is also utilities that combine creation of buffers/images etc. with binding
  // the memory below.
  AllocationID alloc(const VkMemoryRequirements& memReqs,
                     VkMemoryPropertyFlags       memProps,
                     bool                        isLinear,  // buffers are linear, optimal tiling textures are not
                     const VkMemoryDedicatedAllocateInfo* dedicated,
                     VkResult&                            result)
  {
    return allocInternal(memReqs, memProps, isLinear, dedicated, result, true, m_defaultState);
  }

  // make individual raw allocations.
  // there is also utilities that combine creation of buffers/images etc. with binding
  // the memory below.
  AllocationID alloc(const VkMemoryRequirements& memReqs,
                     VkMemoryPropertyFlags       memProps,
                     bool                        isLinear,  // buffers are linear, optimal tiling textures are not
                     const VkMemoryDedicatedAllocateInfo* dedicated,
                     State&                               state,
                     VkResult&                            result)
  {
    return allocInternal(memReqs, memProps, isLinear, dedicated, result, true, state);
  }

  AllocationID alloc(const VkMemoryRequirements& memReqs,
                     VkMemoryPropertyFlags       memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     bool isLinear = true,  // buffers are linear, optimal tiling textures are not
                     const VkMemoryDedicatedAllocateInfo* dedicated = nullptr)
  {
    VkResult result;
    return allocInternal(memReqs, memProps, isLinear, dedicated, result, true, m_defaultState);
  }

  // unless you use the freeAll mechanism, each allocation must be freed individually
  void free(AllocationID allocationID);

  // returns the detailed information from an allocationID
  const Allocation& getAllocation(AllocationID id) const;

  // can have multiple map/unmaps at once, but must be paired
  // internally will keep the vk mapping active as long as one map is active
  void* map(AllocationID allocationID, VkResult *pResult = nullptr);
  void  unmap(AllocationID allocationID);

  template <class T>
  T* mapT(AllocationID allocationID, VkResult *pResult = nullptr)
  {
    return (T*)map(allocationID, pResult);
  }

  //////////////////////////////////////////////////////////////////////////

  // utility functions to create resources and bind their memory directly

  // subsequent creates will use dedicated allocations (mostly for debugging purposes)
  inline void setForceDedicatedAllocation(bool state) { m_forceDedicatedAllocation = state; }
  // subsequent createBuffers will also use these flags
  inline void setDefaultBufferUsageFlags(VkBufferUsageFlags usage) { m_defaultBufferUsageFlags = usage; }

  VkImage createImage(const VkImageCreateInfo& createInfo, AllocationID& allocationID, VkMemoryPropertyFlags memProps, VkResult& result);
  VkImage createImage(const VkImageCreateInfo& createInfo, AllocationID& allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    VkResult result;
    return createImage(createInfo, allocationID, memProps, result);
  }
  VkImage createImage(const VkImageCreateInfo& createInfo, VkMemoryPropertyFlags memProps, VkResult& result)
  {
    AllocationID id;
    return createImage(createInfo, id, memProps, result);
  }
  VkImage createImage(const VkImageCreateInfo& createInfo, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    AllocationID id;
    return createImage(createInfo, id, memProps);
  }


  VkBuffer createBuffer(const VkBufferCreateInfo& createInfo, AllocationID& allocationID, VkMemoryPropertyFlags memProps, VkResult& result);
  VkBuffer createBuffer(const VkBufferCreateInfo& createInfo, AllocationID& allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    VkResult result;
    return createBuffer(createInfo, allocationID, memProps, result);
  }
  VkBuffer createBuffer(const VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags memProps, VkResult& result)
  {
    AllocationID id;
    return createBuffer(createInfo, id, memProps, result);
  }
  VkBuffer createBuffer(const VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    AllocationID id;
    return createBuffer(createInfo, id, memProps);
  }

  VkBuffer createBuffer(VkDeviceSize size,
                        VkBufferUsageFlags usage,  // combined with m_defaultBufferUsageFlags and VK_BUFFER_USAGE_TRANSFER_DST_BIT
                        AllocationID&         allocationID,
                        VkMemoryPropertyFlags memProps,
                        VkResult&             result);
  VkBuffer createBuffer(VkDeviceSize size,
                        VkBufferUsageFlags usage,  // combined with m_defaultBufferUsageFlags and VK_BUFFER_USAGE_TRANSFER_DST_BIT
                        AllocationID&         allocationID,
                        VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    VkResult result;
    return createBuffer(size, usage, allocationID, memProps, result);
  }

#if VK_NV_ray_tracing
  VkAccelerationStructureNV createAccStructure(const VkAccelerationStructureCreateInfoNV& createInfo,
                                               AllocationID&                              allocationID,
                                               VkMemoryPropertyFlags                      memProps,
                                               VkResult&                                  result);
  VkAccelerationStructureNV createAccStructure(const VkAccelerationStructureCreateInfoNV& createInfo,
                                               AllocationID&                              allocationID,
                                               VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    VkResult result;
    return createAccStructure(createInfo, allocationID, memProps, result);
  }
#endif


protected:
  static const VkMemoryDedicatedAllocateInfo* DEDICATED_PROXY;
  static int                                  s_allocDebugBias;

  struct BlockID
  {
    uint32_t index      = INVALID_ID_INDEX;
    uint32_t generation = 0;

    bool     isEqual(const BlockID& other) const { return index == other.index && generation == other.generation; }
    uint32_t instantiate(uint32_t newIndex)
    {
      uint32_t oldIndex = index;
      index             = newIndex;
      generation++;

      return oldIndex;
    }

    friend bool operator==(const BlockID& lhs, const BlockID& rhs) { return rhs.isEqual(lhs); }
  };

  struct Block
  {
    BlockID                   id{};  // index to self, or next free item
    VkDeviceMemory            mem = VK_NULL_HANDLE;
    nvh::TRangeAllocator<256> range;

    VkDeviceSize allocationSize = 0;
    VkDeviceSize usedSize       = 0;

    // to avoid management of pages via limits::bufferImageGranularity,
    // a memory block is either fully linear, or non-linear
    bool                  isLinear    = false;
    bool                  isDedicated = false;
    bool                  isFirst     = false;  // first memory block of a type
    float                 priority    = 0.0f;
    VkMemoryAllocateFlags allocateFlags{};
    uint32_t              allocateDeviceMask = 0;

    uint32_t memoryTypeIndex = 0;
    uint32_t allocationCount = 0;
    uint32_t mapCount        = 0;
    uint32_t mappable        = 0;
    uint8_t* mapped          = nullptr;

    Block& operator=(Block&&) = default;
    Block(Block&&)            = default;
    Block(const Block&)       = default;
    Block()                   = default;
  };

  struct AllocationInfo
  {
    AllocationID id{};  // index to self, or next free item
    Allocation   allocation{};
    uint32_t     blockOffset = 0;
    uint32_t     blockSize   = 0;
    BlockID      block{};
  };

  VkDevice     m_device            = VK_NULL_HANDLE;
  VkDeviceSize m_blockSize         = 0;
  VkDeviceSize m_allocatedSize     = 0;
  VkDeviceSize m_usedSize          = 0;
  VkDeviceSize m_maxAllocationSize = 0;

  std::vector<Block>          m_blocks;
  std::vector<AllocationInfo> m_allocations;

  // linked-list to next free allocation
  uint32_t m_freeAllocationIndex = INVALID_ID_INDEX;
  // linked-list to next free block
  uint32_t m_freeBlockIndex   = INVALID_ID_INDEX;
  uint32_t m_activeBlockCount = 0;

  VkPhysicalDeviceMemoryProperties m_memoryProperties;
  VkPhysicalDevice                 m_physicalDevice = NULL;

  State              m_defaultState;

  VkBufferUsageFlags m_defaultBufferUsageFlags  = 0;
  bool               m_forceDedicatedAllocation = false;
  bool               m_supportsPriority         = false;
  // heuristic that doesn't immediately free the first memory block of a specific memorytype
  bool m_keepFirst = true;

  std::string m_debugName;

  AllocationID allocInternal(const VkMemoryRequirements& memReqs,
                             VkMemoryPropertyFlags       memProps,
                             bool isLinear,  // buffers are linear, optimal tiling textures are not
                             const VkMemoryDedicatedAllocateInfo* dedicated,
                             VkResult&                            result,
                             bool                                 preferDevice,
                             const State&                         state);

  AllocationID createID(Allocation& allocation, BlockID block, uint32_t blockOffset, uint32_t blockSize);
  void         destroyID(AllocationID id);

  const AllocationInfo& getInfo(AllocationID id) const
  {
    assert(m_allocations[id.index].id.isEqual(id));

    return m_allocations[id.index];
  }

  Block& getBlock(BlockID id)
  {
    Block& block = m_blocks[id.index];
    assert(block.id.isEqual(id));
    return block;
  }

  //////////////////////////////////////////////////////////////////////////
  // For derived memory allocators you can do special purpose operations via overloading these functions.
  // A typical use-case would be export/import the memory to another API.

  virtual VkResult allocBlockMemory(BlockID id, VkMemoryAllocateInfo& memInfo, VkDeviceMemory& deviceMemory)
  {
    //s_allocDebugBias++;
    return vkAllocateMemory(m_device, &memInfo, nullptr, &deviceMemory);
  }
  virtual void freeBlockMemory(BlockID id, VkDeviceMemory deviceMemory)
  {
    //s_allocDebugBias--;
    vkFreeMemory(m_device, deviceMemory, nullptr);
  }
  virtual void resizeBlocks(uint32_t count) {}

  virtual VkResult createBufferInternal(VkDevice device, const VkBufferCreateInfo* info, VkBuffer* buffer)
  {
    return vkCreateBuffer(device, info, nullptr, buffer);
  }

  virtual VkResult createImageInternal(VkDevice device, const VkImageCreateInfo* info, VkImage* image)
  {
    return vkCreateImage(device, info, nullptr, image);
  }
};

}  // namespace nvvk
