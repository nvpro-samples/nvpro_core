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

#pragma once

#include <assert.h>
#include <platform.h>
#include <vector>

#include <nvh/trangeallocator.hpp>
#include <vulkan/vulkan_core.h>

namespace nvvk {


// safe-ish value for most desktop hw http://vulkan.gpuinfo.org/displayextensionproperty.php?name=maxMemoryAllocationSize
#define NVVK_DEFAULT_MAX_MEMORY_ALLOCATIONSIZE (VkDeviceSize(2 * 1024) * 1024 * 1024)

#define NVVK_DEFAULT_MEMORY_BLOCKSIZE (VkDeviceSize(128) * 1024 * 1024)

#define NVVK_DEFAULT_STAGING_BLOCKSIZE (VkDeviceSize(64) * 1024 * 1024)

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
  # class nvvk::DeviceMemoryAllocator

  DeviceMemoryAllocator allocates and manages device memory in fixed-size memory blocks.

  It sub-allocates from the blocks, and can re-use memory if it finds empty
  regions. Because of the fixed-block usage, you can directly create resources
  and don't need a phase to compute the allocation sizes first.

  It will create compatible chunks according to the memory requirements and
  usage flags. Therefore you can easily create mappable host allocations
  and delete them after usage, without inferring device-side allocations.

  We return `AllocationID` rather than the allocation details directly, which
  you can query separately.

  Several utility functions are provided to handle the binding of memory
  directly with the resource creation of buffers, images and acceleration
  structures. This utilities also make implicit use of Vulkan's dedicated
  allocation mechanism.

  > **WARNING** : The memory manager serves as proof of concept for some key concepts
  > however it is not meant for production use and it currently lacks de-fragmentation logic
  > as well. You may want to look at [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
  > for a more production-focused solution.

  You can derive from this calls and overload the 

  Example :
  ~~~ C++
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

  ~~~
*/
class DeviceMemoryAllocator
{

public:
  static const float DEFAULT_PRIORITY;

#ifndef NDEBUG
  virtual ~DeviceMemoryAllocator()
  {
    // If all memory was released properly, no blocks should be alive at this point
    assert(m_blocks.empty());
  }
#endif

  // system related

  DeviceMemoryAllocator() {}
  DeviceMemoryAllocator(VkDevice         device,
                        VkPhysicalDevice physicalDevice,
                        VkDeviceSize     blockSize = NVVK_DEFAULT_MEMORY_BLOCKSIZE,
                        VkDeviceSize     maxSize   = NVVK_DEFAULT_MAX_MEMORY_ALLOCATIONSIZE)
  {
    init(device, physicalDevice, blockSize, maxSize);
  }

  void init(VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     blockSize = NVVK_DEFAULT_MEMORY_BLOCKSIZE,
            VkDeviceSize     maxSize   = NVVK_DEFAULT_MAX_MEMORY_ALLOCATIONSIZE);

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
  
  void getTypeStats(uint32_t count[VK_MAX_MEMORY_TYPES], VkDeviceSize used[VK_MAX_MEMORY_TYPES], VkDeviceSize allocated[VK_MAX_MEMORY_TYPES]) const;

  VkDevice                                getDevice() const;
  VkPhysicalDevice                        getPhysicalDevice() const;
  const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;
  VkDeviceSize                            getMaxAllocationSize() const;

  //////////////////////////////////////////////////////////////////////////

  // subsequent allocations (and creates) will use the provided priority
  // ignored if setPrioritySupported is not enabled
  float setPriority(float priority = DEFAULT_PRIORITY)
  {
    float old  = m_priority;
    m_priority = priority;
    return old;
  }

  float getPriority() const { return m_priority; }

  // make individual raw allocations.
  // there is also utilities that combine creation of buffers/images etc. with binding
  // the memory below.
  AllocationID alloc(const VkMemoryRequirements& memReqs,
                     VkMemoryPropertyFlags       memProps,
                     bool                        isLinear,  // buffers are linear, optimal tiling textures are not
                     const VkMemoryDedicatedAllocateInfo* dedicated,
                     VkResult&                            result)
  {
    return allocInternal(memReqs, memProps, isLinear, dedicated, result, true);
  }

  AllocationID alloc(const VkMemoryRequirements& memReqs,
                     VkMemoryPropertyFlags       memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     bool isLinear = true,  // buffers are linear, optimal tiling textures are not
                     const VkMemoryDedicatedAllocateInfo* dedicated = nullptr)
  {
    VkResult result;
    return allocInternal(memReqs, memProps, isLinear, dedicated, result, true);
  }

  // unless you use the freeAll mechanism, each allocation must be freed individually
  void free(AllocationID allocationID);

  // returns the detailed information from an allocationID
  const Allocation& getAllocation(AllocationID id) const;

  // can have multiple map/unmaps at once, but must be paired
  // internally will keep the vk mapping active as long as one map is active
  void* map(AllocationID allocationID);
  void  unmap(AllocationID allocationID);

  template <class T>
  T* mapT(AllocationID allocationID)
  {
    return (T*)map(allocationID);
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

  VkBuffer createBuffer(const VkBufferCreateInfo& createInfo, AllocationID& allocationID, VkMemoryPropertyFlags memProps, VkResult& result);
  VkBuffer createBuffer(const VkBufferCreateInfo& createInfo, AllocationID& allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  {
    VkResult result;
    return createBuffer(createInfo, allocationID, memProps, result);
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
    BlockID                   id;  // index to self, or next free item
    VkDeviceMemory            mem = VK_NULL_HANDLE;
    nvh::TRangeAllocator<256> range;

    VkDeviceSize allocationSize;
    VkDeviceSize usedSize;

    // to avoid management of pages via limits::bufferImageGranularity,
    // a memory block is either fully linear, or non-linear
    bool  isLinear;
    bool  isDedicated;
    bool  isFirst;  // first memory block of a type
    float priority;

    uint32_t memoryTypeIndex;
    uint32_t allocationCount;
    uint32_t mapCount;
    uint32_t mappable;
    uint8_t* mapped;

    Block& operator=(Block&&) = default;
    Block(Block&&)            = default;
    Block()                   = default;
  };

  struct AllocationInfo
  {
    AllocationID id;  // index to self, or next free item
    Allocation   allocation;
    uint32_t     blockOffset;
    uint32_t     blockSize;
    BlockID      block;
  };

  VkDevice     m_device            = VK_NULL_HANDLE;
  VkDeviceSize m_blockSize         = 0;
  VkDeviceSize m_allocatedSize     = 0;
  VkDeviceSize m_usedSize          = 0;
  VkDeviceSize m_maxAllocationSize = NVVK_DEFAULT_MAX_MEMORY_ALLOCATIONSIZE;

  std::vector<Block>          m_blocks;
  std::vector<AllocationInfo> m_allocations;

  // linked-list to next free allocation
  uint32_t m_freeAllocationIndex = INVALID_ID_INDEX;
  // linked-list to next free block
  uint32_t m_freeBlockIndex   = INVALID_ID_INDEX;
  uint32_t m_activeBlockCount = 0;

  VkPhysicalDeviceMemoryProperties m_memoryProperties;
  VkPhysicalDevice                 m_physicalDevice = VK_NULL_HANDLE;

  float m_priority = DEFAULT_PRIORITY;


  VkBufferUsageFlags m_defaultBufferUsageFlags  = 0;
  bool               m_forceDedicatedAllocation = false;
  bool               m_supportsPriority         = false;
    // heuristic that doesn't immediately free the first memory block of a specific memorytype
  bool m_keepFirst = true;

  AllocationID allocInternal(const VkMemoryRequirements& memReqs,
                             VkMemoryPropertyFlags       memProps,
                             bool isLinear,  // buffers are linear, optimal tiling textures are not
                             const VkMemoryDedicatedAllocateInfo* dedicated,
                             VkResult&                            result,
                             bool                                 preferDevice);

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
};

//////////////////////////////////////////////////////////////////
/**
  # class nvvk::StagingMemoryManager

  StagingMemoryManager class is a utility that manages host visible
  buffers and their allocations in an opaque fashion to assist
  asynchronous transfers between device and host.

  The collection of such resources is represented by nvvk::StagingID.
  The necessary memory is sub-allocated and recycled in blocks internally.

  The default implementation will create one dedicated memory allocation per block.
  You can derive from this class and overload the virtual functions,
  if you want to use a different allocation system.

  - **allocBlockMemory**
  - **freeBlockMemory**

  > **WARNING:**
  > - cannot manage copy > 4 GB

  Usage:
  - Enqueue transfers into the provided VkCommandBuffer and then finalize the copy operations.
  - You can either signal the completion of the operations manually by the nvvk::StagingID
    or implicitly by associating the copy operations with a VkFence.
  - The release of the resources allows to safely recycle the memory for future transfers.

  Example :

  ~~~ C++
  StagingMemoryManager  staging;
  staging.init(device, physicalDevice);


  // Enqueue copy operations of data to target buffer.
  // This internally manages the required staging resources
  staging.cmdToBuffer(cmd, targetBufer, 0, targetSize, targetData);

  // you can also get access to a temporary mapped pointer and fill
  // the staging buffer directly
  vertices = staging.cmdToBufferT<Vertex>(cmd, targetBufer, 0, targetSize);

  // OPTION A:
  // associate all previous copy operations with a fence
  staging.finalizeCmds(myfence);

  ...

  // every once in a while call
  staging.tryReleaseFenced();

  // OPTION B
  // alternatively manage the resources and fence waiting yourself
  sid = staging.finalizeCmds();

  ... need to ensure uploads completed

  staging.recycleTask(sid);

  ~~~
*/

class StagingID
{
  friend class StagingMemoryManager;

public:
  bool isValid() const { return index != INVALID_ID_INDEX; }
  bool isEqual(const StagingID& other) const { return index == other.index && generation == other.generation; }

  operator bool() const { return isValid(); }

  friend bool operator==(const StagingID& lhs, const StagingID& rhs) { return rhs.isEqual(lhs); }

private:
  uint32_t index      = INVALID_ID_INDEX;
  uint32_t generation = 0;

  void     invalidate() { index = INVALID_ID_INDEX; }
  uint32_t instantiate(uint32_t inIndex)
  {
    uint32_t oldIndex = index;
    index             = inIndex;
    generation++;
    return oldIndex;
  }
};

class StagingMemoryManager
{
public:
  //////////////////////////////////////////////////////////////////////////

  StagingMemoryManager() {}
  StagingMemoryManager(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
  {
    init(device, physicalDevice, stagingBlockSize);
  }

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  void deinit();

  // if true (default) we free the memory completely when released
  // otherwise we would keep blocks for re-use around, unless freeUnused() is called
  void setFreeUnusedOnRelease(bool state) { m_freeOnRelease = state; }

  // test if there is enough space in current allocations
  bool fitsInAllocated(VkDeviceSize size) const;

  // if data != nullptr memcpies to mapping and returns nullptr
  // otherwise returns temporary mapping (valid until "complete" functions)
  void* cmdToImage(VkCommandBuffer                 cmd,
                   VkImage                         image,
                   const VkOffset3D&               offset,
                   const VkExtent3D&               extent,
                   const VkImageSubresourceLayers& subresource,
                   VkDeviceSize                    size,
                   const void*                     data);


  // if data != nullptr memcpies to mapping and returns nullptr
  // otherwise returns temporary mapping (valid until "complete" functions)
  void* cmdToBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

  template <class T>
  T* cmdToBufferT(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
  {
    return (T*)cmdToBuffer(cmd, buffer, offset, size, nullptr);
  }

  // provides a handle for all staging resources by previous cmd operations
  // since the last finalize.
  // fence is optional, allows the usage of tryReleaseFenced
  StagingID finalizeCmds(VkFence fence = VK_NULL_HANDLE);

  // signals that outstanding copy operations have completed
  // allows to reclaim internal staging memory
  void release(StagingID stagingID);

  // completes all tasks whose fence was triggered
  void tryReleaseFenced();

  // frees staging memory no longer in use
  void freeUnused() { free(true); }

  float getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const;

protected:
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
    BlockID                   id;  // index to self, or next free item
    VkDeviceSize              size   = 0;
    VkBuffer                  buffer = VK_NULL_HANDLE;
    VkDeviceMemory            memory = VK_NULL_HANDLE;
    nvh::TRangeAllocator<256> range;
    uint8_t*                  mapping;
  };

  struct Entry
  {
    BlockID  block;
    uint32_t offset;
    uint32_t size;
  };

  struct StagingSet
  {
    StagingID          id;  // index to self, or next free item
    VkFence            fence = VK_NULL_HANDLE;
    std::vector<Entry> entries;
  };

  VkDevice         m_device           = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice   = VK_NULL_HANDLE;
  uint32_t         m_memoryTypeIndex  = ~0;
  VkDeviceSize     m_stagingBlockSize = 0;
  bool             m_freeOnRelease    = true;

  std::vector<Block>      m_blocks;
  std::vector<StagingSet> m_sets;

  // linked-list to next free staging set
  uint32_t m_freeStagingIndex = INVALID_ID_INDEX;
  // linked list to next free block
  uint32_t m_freeBlockIndex = INVALID_ID_INDEX;

  VkDeviceSize m_allocatedSize = 0;
  VkDeviceSize m_usedSize      = 0;

  StagingID m_current;

  void free(bool unusedOnly);
  void freeBlock(Block& block);

  StagingID createID();

  void* getStagingSpace(VkDeviceSize size, VkBuffer& buffer, VkDeviceSize& offset);

  Block& getBlock(BlockID id)
  {
    Block& block = m_blocks[id.index];
    assert(block.id.isEqual(id));
    return block;
  }

  //////////////////////////////////////////////////////////////////////////
  // You can specialize the staging buffer allocation mechanism used.
  // The default is using one dedicated VkDeviceMemory per staging VkBuffer.

  // must fill block.buffer, memory, mapping
  virtual VkResult allocBlockMemory(BlockID id, VkDeviceSize size, bool toDevice, Block& block);
  virtual void     freeBlockMemory(BlockID id, const Block& block);
  virtual void     resizeBlocks(uint32_t num) {}
};

class ScopeStagingMemoryManager : public StagingMemoryManager
{
public:
  ScopeStagingMemoryManager(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
  {
    init(device, physicalDevice, stagingBlockSize);
  }

  ~ScopeStagingMemoryManager()
  {
    vkDeviceWaitIdle(m_device);
    deinit();
  }
};

//////////////////////////////////////////////////////////////////
/**
  # class nvvk::StagingMemoryManagerDma

  Derives from nvvk::StagingMemoryManager and uses the referenced nvvk::DeviceMemoryAllocator
  for allocations.

  ~~~ C++
  DeviceMemoryAllocator    memAllocator;
  memAllocator.init(device, physicalDevice);

  StagingMemoryManagerDma  staging;
  staging.init(memAllocator);

  // rest as usual
  staging.cmdToBuffer(cmd, targetBufer, 0, targetSize, targetData);
  ~~~
*/

class StagingMemoryManagerDma : public StagingMemoryManager
{
public:
  StagingMemoryManagerDma(DeviceMemoryAllocator& memAllocator, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
  {
    init(memAllocator, stagingBlockSize);
  }
  StagingMemoryManagerDma() {}

  void init(DeviceMemoryAllocator& memAllocator, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
  {
    StagingMemoryManager::init(memAllocator.getDevice(), memAllocator.getPhysicalDevice(), stagingBlockSize);
    m_memAllocator = &memAllocator;
  }


protected:
  DeviceMemoryAllocator*    m_memAllocator;
  std::vector<AllocationID> m_blockAllocs;

  VkResult allocBlockMemory(BlockID id, VkDeviceSize size, bool toDevice, Block& block) override
  {
    VkResult result;
    float    priority = m_memAllocator->setPriority();
    block.buffer      = m_memAllocator->createBuffer(
        size, toDevice ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_blockAllocs[id.index],
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | (toDevice ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_HOST_CACHED_BIT),
        result);
    m_memAllocator->setPriority(priority);
    if(result == VK_SUCCESS)
    {
      block.mapping = m_memAllocator->mapT<uint8_t>(m_blockAllocs[id.index]);
    }

    return result;
  }
  void freeBlockMemory(BlockID id, const Block& block) override
  {
    vkDestroyBuffer(m_device, block.buffer, nullptr);
    m_memAllocator->unmap(m_blockAllocs[id.index]);
    m_memAllocator->free(m_blockAllocs[id.index]);
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

}  // namespace nvvk
