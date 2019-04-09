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

namespace nvvk
{
  //////////////////////////////////////////////////////////////////////////
  #define NVVK_DEFAULT_MAX_MEMORYALLOCATIONSIZE  (VkDeviceSize(2) * 1024 * 1024 * 1024)
  // safeish value for most desktop hw http://vulkan.gpuinfo.org/displayextensionproperty.php?name=maxMemoryAllocationSize

  struct Allocation
  {
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDeviceSize   offset = 0;
    VkDeviceSize   size = 0;
  };

  class AllocationID
  {
    friend class BlockDeviceMemoryAllocator;

  private:
    uint32_t index = ~0;
    uint32_t incarnation = 0;

  public:
    void invalidate() { index = ~0; }
    bool isValid() const { return index != ~0; }
    bool isEqual(const AllocationID& other) const { return index == other.index && incarnation == other.incarnation; }
    operator bool() const { return isValid(); }
  };

  //////////////////////////////////////////////////////////////////////////

  class MemoryBlockManager
  {
  public:

    /*
      This class allows to find out how much device memory (organized in a minimal amount of blocks with an upper size limit)
      are required for a given set of resources. It sub-allocates those resources from the blocks
      and gives back the appropriate ranges and which block is used.

      The class does NOT actually allocate any device memory at all. It just provides
      information how those allocations should be made and where each resource finds itself.

      The idea is that you allocate a tight set (no-waste) of device memory after you filled the block manager.
      It assumes resources are never deleted individually, but if so, alls block are destroyed.
    */

    struct Entry
    {
      size_t       blockIdx = ~0ULL;
      VkDeviceSize offset = 0;

      bool isValid() const {
        return blockIdx != ~0ULL;
      }
    };

    struct Block
    {
      // use whatever is preferred for doing actual allocation
      VkMemoryAllocateInfo  memAllocate;
      VkMemoryRequirements  memReqs;
      VkMemoryPropertyFlags memProps;
    };

    void init(const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize = 32 * 1024 * 1024, VkDeviceSize maxAllocationSize = NVVK_DEFAULT_MAX_MEMORYALLOCATIONSIZE);

    // appends to existing allocation blocks if currentUsage < blockSize, starts new block otherwise
    // no memory is wasted
    Entry add(const VkMemoryRequirements& memReqs, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    void  clear();

    const size_t getBlockCount() const;
    const Block& getBlock(size_t idx) const;

    MemoryBlockManager() {}
    MemoryBlockManager(const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize = 32 * 1024 * 1024, VkDeviceSize maxAllocationSize = NVVK_DEFAULT_MAX_MEMORYALLOCATIONSIZE)
    {
      init(memoryProperties, blockSize, maxAllocationSize);
    }

    // utility helper when used with MemoryBlockAllocation
    VkBindImageMemoryInfo makeBind(VkImage image, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const;
    VkBindBufferMemoryInfo makeBind(VkBuffer buffer, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const;
#if VK_NV_ray_tracing
    VkBindAccelerationStructureMemoryInfoNV makeBind(VkAccelerationStructureNV        accel,
      const MemoryBlockManager::Entry& entry,
      const Allocation*                blockAllocations) const;
#endif

  private:
    const VkPhysicalDeviceMemoryProperties* m_memoryProperties;
    VkDeviceSize                            m_maxAllocationSize;
    VkDeviceSize                            m_blockSize;
    std::vector<Block>                      m_blocks;
  };

  //////////////////////////////////////////////////////////////////////////


  class BlockDeviceMemoryAllocator
  {
    /*
      This class allocates and manages device memory in fixed-size blocks.

      It sub-allocates from the blocks, and can re-use memory if it finds empty
      regions. The logic is very basic and should not be used in production
      code, it currently lacks de-fragmentation or smarter re-use of gaps in the memory.

      Because of the fixed-block usage, you can directly create resources and 
      don't need a phase to compute the allocation sizes first.

      It will create compatible chunks according to the memory requirements and 
      usage flags. Therefore you can easily create mappable host allocations
      and delete them after usage, without inferring device-side allocations.

      We return allocationIDs rather than the allocation details directly,
      as a future variant may allow de-fragmentation.
    */

  public:
#ifndef NDEBUG
    ~BlockDeviceMemoryAllocator()
    {
      // If all memory was released properly, no blocks should be alive at this point
      assert(m_blocks.empty());
    }
#endif

    //////////////////////////////////////////////////////////////////////////

    // system related

    void init(VkDevice                        device,
      const VkPhysicalDeviceMemoryProperties* memoryProperties,
      VkDeviceSize                            blockSize = 1024 * 1024 * 32,
      const VkAllocationCallbacks*            allocator = nullptr);

    // frees all blocks independent of individual allocations
    void freeAll();
    void deinit();

    void setMaxAllocationSize(VkDeviceSize size);

    float getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const;

    VkDevice                                getDevice() const;
    const VkAllocationCallbacks*            getAllocationCallbacks() const;
    const VkPhysicalDeviceMemoryProperties* getMemoryProperties() const;
    VkDeviceSize                            getMaxAllocationSize() const;

    //////////////////////////////////////////////////////////////////////////

    // make individual allocations.


    AllocationID alloc(const VkMemoryRequirements&          memReqs,
      VkMemoryPropertyFlags                memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      const VkMemoryDedicatedAllocateInfo* dedicated = nullptr);
    void         free(AllocationID allocationID);

    // returns the detailed information from an allocationID
    Allocation getAllocation(AllocationID id) const;

    // can have multiple map/unmaps at once, but must be paired
    // internally will keep the vk mapping active as long as one map is active
    void* map(AllocationID allocationID);
    void  unmap(AllocationID allocationID);

    template <class T>
    T* tmap(AllocationID allocationID)
    {
      return (T*)map(allocationID);
    }


    //////////////////////////////////////////////////////////////////////////

    // utility functions to create resources and bind their memory

    VkResult create(const VkImageCreateInfo& createInfo,
      VkImage&                 image,
      AllocationID&            allocationID,
      VkMemoryPropertyFlags    memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      bool                     useDedicated = false);

    VkResult create(const VkBufferCreateInfo& createInfo,
      VkBuffer&                 buffer,
      AllocationID&             allocationID,
      VkMemoryPropertyFlags     memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      bool                      useDedicated = false);

#if VK_NV_ray_tracing
    VkResult create(const VkAccelerationStructureCreateInfoNV& createInfo,
      VkAccelerationStructureNV&                 accel,
      AllocationID&                              allocationID,
      VkMemoryPropertyFlags                      memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      bool                                       useDedicated = false);
#endif

  private:

    struct MemoryBlock
    {
      VkDeviceMemory mem;
      VkDeviceSize   currentOffset;
      VkDeviceSize   allocationSize;
      uint32_t       memoryTypeIndex;
      uint32_t       allocationCount;
      uint32_t       mapCount;
      uint32_t       mappable;
      uint8_t*       mapped;
    };

    typedef std::vector<MemoryBlock>::iterator MemoryBlockIterator;

    struct AllocationInfo
    {
      Allocation   allocation;
      AllocationID id;
    };

    struct CompactBind
    {
      AllocationID                    id;
      nvvk::MemoryBlockManager::Entry entry;
      VkStructureType                 type;
      union
      {
        VkImage  image;
        VkBuffer buffer;
      };
    };

    VkDevice     m_device            = VK_NULL_HANDLE;
    VkDeviceSize m_blockSize         = 0;
    VkDeviceSize m_allocatedSize     = 0;
    VkDeviceSize m_usedSize          = 0;
    VkDeviceSize m_maxAllocationSize = NVVK_DEFAULT_MAX_MEMORYALLOCATIONSIZE;

    std::vector<MemoryBlock>    m_blocks;
    std::vector<AllocationInfo> m_allocations;

    const VkAllocationCallbacks*            m_allocation_callbacks = nullptr;
    const VkPhysicalDeviceMemoryProperties* m_memoryProperties     = nullptr;


    AllocationID        createID(Allocation& allocation);
    void                freeID(AllocationID id);
    MemoryBlockIterator getBlock(const Allocation& allocation);
  };

}

