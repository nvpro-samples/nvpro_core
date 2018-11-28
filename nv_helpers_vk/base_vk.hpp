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

#ifndef NV_VK_BASE_INCLUDED
#define NV_VK_BASE_INCLUDED

#include <platform.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace nv_helpers_vk {

  /*
    WARNING

    These functions and classes serve as a convenience layer to use Vulkan 
    for creating sample applications.

    They are NOT MEANT for high-performance use-cases!

    FIXME split into multiple files
  */


  //////////////////////////////////////////////////////////////////////////

  // returns ~0 if not found
  uint32_t PhysicalDeviceMemoryProperties_getMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties, const VkMemoryRequirements &memReqs, VkFlags memProps);

  // returns false on incompatibility
  bool PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties, 
    const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfo);

  bool PhysicalDeviceMemoryProperties_appendMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
    const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfoAppended, VkDeviceSize &offset);

  //////////////////////////////////////////////////////////////////////////

  struct PhysicalInfo 
  {
    VkPhysicalDevice                      physicalDevice;
    std::vector<VkPhysicalDevice>         physicalDeviceGroup;
    VkPhysicalDeviceMemoryProperties      memoryProperties;
    VkPhysicalDeviceProperties            properties;   // copy of properties2.properties (backwards compatibility)
    VkPhysicalDeviceFeatures2             features2;
    std::vector<VkQueueFamilyProperties>  queueProperties;

    // vulkan 1.1
    struct Features {
      VkPhysicalDeviceMultiviewFeatures               multiview;
      VkPhysicalDevice16BitStorageFeatures            t16BitStorage;
      VkPhysicalDeviceSamplerYcbcrConversionFeatures  samplerYcbcrConversion;
      VkPhysicalDeviceProtectedMemoryFeatures         protectedMemory;
      VkPhysicalDeviceShaderDrawParameterFeatures     drawParameters;
      VkPhysicalDeviceVariablePointerFeatures         variablePointers;
    };
    struct Properties {
      VkPhysicalDeviceMaintenance3Properties          maintenance3;
      VkPhysicalDeviceIDProperties                    deviceID;
      VkPhysicalDeviceMultiviewProperties             multiview;
      VkPhysicalDeviceProtectedMemoryProperties       protectedMemory;
      VkPhysicalDevicePointClippingProperties         pointClipping;
      VkPhysicalDeviceSubgroupProperties              subgroup;
    };

    Features                              extFeatures;
    Properties                            extProperties;

    void init(VkPhysicalDevice physicalDeviceIn, uint32_t apiMajor=1, uint32_t apiMinor=0);

    bool      getOptimalDepthStencilFormat(VkFormat &depthStencilFormat) const;
    uint32_t  getExclusiveQueueFamily(VkQueueFlagBits bits) const;
    uint32_t  getQueueFamily(VkQueueFlags bits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT) const;
    uint32_t  getPresentQueueFamily(VkSurfaceKHR surface, VkQueueFlags bits=VK_QUEUE_GRAPHICS_BIT) const;

    bool  getMemoryAllocationInfo(const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfo) const;
    bool  appendMemoryAllocationInfo(const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfoAppended, VkDeviceSize &offset) const;

    PhysicalInfo() {}
    PhysicalInfo(VkPhysicalDevice physical, uint32_t apiMajor = 1, uint32_t apiMinor = 0) 
    {
      init(physical, apiMajor, apiMinor);
    }
  };

  //////////////////////////////////////////////////////////////////////////

  class Submission 
  {
  private:
    VkQueue                           m_queue = nullptr;
    std::vector<VkSemaphore>          m_waits;
    std::vector<VkPipelineStageFlags> m_waitFlags;
    std::vector<VkSemaphore>          m_signals;
    std::vector<VkCommandBuffer>      m_commands;

  public:

    uint32_t  getCommandBufferCount() const;

    // can change queue if nothing is pending
    void    setQueue(VkQueue queue);

    void    enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers);
    void    enqueue(VkCommandBuffer cmdbuffer);
    void    enqueueAt(uint32_t pos, VkCommandBuffer cmdbuffer);
    void    enqueueSignal(VkSemaphore sem);
    void    enqueueWait(VkSemaphore sem, VkPipelineStageFlags flag);

    // submits the work and resets internal state
    VkResult    execute(VkFence fence = nullptr, uint32_t deviceMask = 0);

  };

  //////////////////////////////////////////////////////////////////////////

  struct TempSubmissionInterface {
    virtual VkCommandBuffer tempSubmissionCreateCommandBuffer(bool primary, VkQueueFlags preferredQueue=0) = 0;
    virtual void            tempSubmissionEnqueue(VkCommandBuffer cmd, VkQueueFlags preferredQueue=0) = 0;
    virtual void            tempSubmissionSubmit(bool sync, VkFence fence = 0, VkQueueFlags preferredQueue = 0, uint32_t deviceMask = 0) = 0;
  };

  //////////////////////////////////////////////////////////////////////////

  struct Makers {
    // implicitly sets VK_BUFFER_USAGE_TRANSFER_DST_BIT
    static VkBufferCreateInfo            makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0);
    static VkBufferViewCreateInfo        makeBufferViewCreateInfo(VkDescriptorBufferInfo &descrInfo, VkFormat fmt, VkBufferViewCreateFlags flags = 0);
    static VkDescriptorBufferInfo        makeDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0);
    static VkDescriptorSetLayoutBinding  makeDescriptorSetLayoutBinding(VkDescriptorType type, VkPipelineStageFlags flags, uint32_t bindingSlot = 0, const VkSampler * pSamplers = nullptr, uint32_t count=1);

    static uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask);

    // assumes full array provided
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorImageInfo *pImageInfo);
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo);
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView);
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const void* pNext);

    // single array entry
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorImageInfo *pImageInfo);
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorBufferInfo* pBufferInfo);
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView);
    static VkWriteDescriptorSet makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const void* pNext);
  };

  //////////////////////////////////////////////////////////////////////////

  struct DeviceUtils : public Makers
  {
    // FIXME remove Makers
    DeviceUtils() : m_device(VK_NULL_HANDLE), m_allocator(nullptr) {}
    DeviceUtils(VkDevice device, const VkAllocationCallbacks* allocator = nullptr) : m_device(device), m_allocator(allocator) {}

    VkDevice                      m_device;
    const VkAllocationCallbacks*  m_allocator;

    VkResult                allocMemAndBindBuffer(VkBuffer obj, const VkPhysicalDeviceMemoryProperties& memoryProperties, VkDeviceMemory &gpuMem, VkFlags memProps);

    VkBuffer                createBuffer(size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0) const;
    VkBufferView            createBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize size, VkDeviceSize offset = 0, VkBufferViewCreateFlags flags = 0) const;
    VkBufferView            createBufferView(VkDescriptorBufferInfo info, VkFormat format, VkBufferViewCreateFlags flags = 0) const;

    VkDescriptorSetLayout         createDescriptorSetLayout(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSetLayoutCreateFlags flags = 0) const;
    VkPipelineLayout              createPipelineLayout(size_t numLayouts, const VkDescriptorSetLayout* setLayouts, size_t numRanges = 0, const VkPushConstantRange* ranges = nullptr) const;

    void  createDescriptorPoolAndSets(uint32_t maxSets, size_t poolSizeCount, const VkDescriptorPoolSize* poolSizes,
      VkDescriptorSetLayout layout, VkDescriptorPool& pool, VkDescriptorSet* sets) const;
  };

  //////////////////////////////////////////////////////////////////////////

  static const uint32_t MAX_RING_FRAMES = 3;

  class RingFences
  {
  public:
    void init(VkDevice device, const VkAllocationCallbacks* allocator = nullptr);
    void deinit();
    void reset();

    // waits until current cycle can be safely used
    // can call multiple times, will skip wait if already used in same frame
    void wait(uint64_t timeout = ~0ULL);

    // query current cycle index
    uint32_t getCycleIndex() const {
      return m_frame % MAX_RING_FRAMES;
    }

    // call once per cycle at end of frame
    VkFence advanceCycle();

  private:
    uint32_t  m_frame;
    uint32_t  m_waited;
    VkFence   m_fences[MAX_RING_FRAMES];
    VkDevice  m_device;
    const VkAllocationCallbacks* m_allocator;
  };

  class RingCmdPool
  {
  public:
    void init(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, const VkAllocationCallbacks* allocator = nullptr);
    void deinit();
    void reset(VkCommandPoolResetFlags flags = 0);

    // call once per cycle prior creating command buffers
    // resets old pools etc.
    void setCycle(uint32_t cycleIndex);

    // ensure proper cycle is set prior this
    VkCommandBuffer         createCommandBuffer(VkCommandBufferLevel level);

    // pointer is only valid until next create
    const VkCommandBuffer*  createCommandBuffers(VkCommandBufferLevel level, uint32_t count);

  private:
    struct Cycle {
      VkCommandPool                 pool;
      std::vector<VkCommandBuffer>  cmds;
    };

    Cycle                         m_cycles[MAX_RING_FRAMES];
    VkDevice                      m_device;
    const VkAllocationCallbacks*  m_allocator;
    uint32_t                      m_index;
    uint32_t                      m_dirty;
  };

  //////////////////////////////////////////////////////////////////////////

  class BasicStagingBuffer {
  public:
    // generic interface which assumes the implementor to flush the staging buffer
    struct Interface {
      virtual void flushStaging(nv_helpers_vk::BasicStagingBuffer& staging) = 0;
    };

    // Uses a single memory allocation,
    // therefore operations are only safe
    // once flushed cmd buffer was completed.

    void init(VkDevice device, const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize chunkSize = 1024 * 1024 * 32, const VkAllocationCallbacks* allocator = nullptr);
    void deinit();

    bool canFlush() const {
      return m_used != 0;
    }

    // encodes the vkCmdCopyBuffer commands into the provided buffer
    // and resets the internal usage for further enqueue operations
    void flush(VkCommandBuffer cmd);

    void flush(TempSubmissionInterface* tempIF, bool sync);

    // must flush if true
    bool cannotEnqueue(VkDeviceSize sz) const {
      return (m_used && m_used + sz > m_available);
    }

    void enqueue(VkImage image, const VkOffset3D &offset, const VkExtent3D &extent, const VkImageSubresourceLayers &subresource, VkDeviceSize size, const void* data);

    void enqueue(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

  private:

    void allocateBuffer(VkDeviceSize size);

    VkBuffer                  m_buffer;
    uint8_t*                  m_mapping;
    VkDeviceSize              m_used;
    VkDeviceSize              m_available;
    VkDeviceSize              m_chunkSize;
    VkDeviceMemory            m_mem;

    std::vector<VkImage>                m_targetImages;
    std::vector<VkBufferImageCopy>      m_targetImageCopies;
    std::vector<VkBuffer>               m_targetBuffers;
    std::vector<VkBufferCopy>           m_targetBufferCopies;

    VkDevice                                  m_device;
    const VkPhysicalDeviceMemoryProperties*   m_memoryProperties;
    const VkAllocationCallbacks*              m_allocator;
  };

  //////////////////////////////////////////////////////////////////////////

  struct Allocation
  {
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
  };

  class AllocationID{
  friend class BasicDeviceMemoryAllocator;
  private:
    uint32_t index = ~0;
    uint32_t incarnation = 0;

  public:
    void invalidate()
    {
      index = ~0;
    }
    bool isValid() const
    {
      return index != ~0;
    }
    bool isEqual(const AllocationID& other) const
    {
      return index == other.index && incarnation == other.incarnation;
    }
    operator bool() const
    {
      return isValid();
    }
  };

  //////////////////////////////////////////////////////////////////////////

  class MemoryBlockManager {
  public:
    struct Entry {
      size_t          blockIdx = ~0;
      VkDeviceSize    offset = 0;
    };

    struct Block {
      // use whatever is preferred for doing actual allocation
      VkMemoryAllocateInfo  memAllocate;
      VkMemoryRequirements  memReqs;
      VkMemoryPropertyFlags memProps;
    };

    void  init(const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize = 32 * 1024 * 1024);

    // appends to existing allocation blocks if currentUsage < blockSize, starts new block otherwise
    // no memory is wasted
    Entry add(const VkMemoryRequirements &memReqs, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    void  clear();

    const size_t getBlockCount() const;
    const Block& getBlock(size_t idx) const;

    MemoryBlockManager() {}
    MemoryBlockManager(const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize = 32 * 1024 * 1024) {
      init(memoryProperties, blockSize);
    }

    // utility helper when used with MemoryBlockAllocation
    VkBindImageMemoryInfo   makeBind(VkImage image, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const;
    VkBindBufferMemoryInfo  makeBind(VkBuffer buffer, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const;
  #if VK_NVX_raytracing
    VkBindAccelerationStructureMemoryInfoNVX makeBind(VkAccelerationStructureNVX accel, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const;
  #endif
  #if VK_NV_ray_tracing
    VkBindAccelerationStructureMemoryInfoNV makeBind(VkAccelerationStructureNV accel, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const;
  #endif

  private:
    const VkPhysicalDeviceMemoryProperties* m_memoryProperties;
    VkDeviceSize                            m_blockSize;
    std::vector<Block>                      m_blocks;

  };

  //////////////////////////////////////////////////////////////////////////


  class BasicDeviceMemoryAllocator
  {
    struct MemoryBlock
    {
      VkDeviceMemory mem;
      VkDeviceSize  currentOffset;
      VkDeviceSize  allocationSize;
      uint32_t      memoryTypeIndex;
      uint32_t      allocationCount;
      uint32_t      mapCount;
      uint32_t      mappable;
      uint8_t*      mapped;
    };

    typedef std::vector<MemoryBlock>::iterator MemoryBlockIterator;

    struct AllocationInfo
    {
      Allocation      allocation;
      AllocationID    id;
    };

    struct CompactBind {
      AllocationID                                id;
      nv_helpers_vk::MemoryBlockManager::Entry entry;
      VkStructureType                             type;
      union {
        VkImage       image;
        VkBuffer      buffer;
      };
    };

    VkDevice      m_device = VK_NULL_HANDLE;
    VkDeviceSize  m_blockSize = 0;
    VkDeviceSize  m_allocatedSize = 0;
    VkDeviceSize  m_usedSize = 0;

    std::vector<MemoryBlock>      m_blocks;
    std::vector<AllocationInfo>   m_allocations;

    const VkAllocationCallbacks *m_allocation_callbacks = nullptr;
    const VkPhysicalDeviceMemoryProperties* m_memoryProperties = nullptr;


    AllocationID createID(Allocation& allocation);
    void freeID(AllocationID id);
    MemoryBlockIterator getBlock(const Allocation& allocation);

  public:
#ifndef NDEBUG
    ~BasicDeviceMemoryAllocator()
    {
      // If all memory was released properly, no blocks should be alive at this point
      assert(m_blocks.empty());
    }
#endif

    // suballocates from blocksized memory or if greater, creates individual memory
    void init(VkDevice device, const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize = 1024 * 1024 * 32, const VkAllocationCallbacks* allocator = nullptr);
    // frees all blocks independent of individual allocations
    void deinit();

    float getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const;

    Allocation getAllocation(AllocationID id) const;
    const VkPhysicalDeviceMemoryProperties* getMemoryProperties() const;

    AllocationID alloc(const VkMemoryRequirements &memReqs, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, const VkMemoryDedicatedAllocateInfo* dedicated = nullptr);
    void  free(AllocationID allocationID);

    // can have multiple map/unmaps at once, but must be paired
    // internally will keep the vk mapping active as long as one map is active
    void* map(AllocationID allocationID);
    void  unmap(AllocationID allocationID);
    
    VkResult create(const VkImageCreateInfo &createInfo, VkImage &image, AllocationID &allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool useDedicated = false);
    VkResult create(const VkBufferCreateInfo &createInfo, VkBuffer &buffer, AllocationID &allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool useDedicated = false);
  #if VK_NVX_raytracing
    VkResult create(const VkAccelerationStructureCreateInfoNVX &createInfo, VkAccelerationStructureNVX &accel, AllocationID &allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool useDedicated = false);
  #endif
  #if VK_NV_ray_tracing
    VkResult create(const VkAccelerationStructureCreateInfoNV &createInfo, VkAccelerationStructureNV &accel, AllocationID &allocationID, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool useDedicated = false);
  #endif
  };
  
  
  //////////////////////////////////////////////////////////////////////////

  template <uint32_t DSETS,uint32_t PIPELAYOUTS=1>
  struct DescriptorPipelineContainer 
  {
    VkPipelineLayout              pipelineLayouts[PIPELAYOUTS];
    VkDescriptorSetLayout         descriptorSetLayout[DSETS];
    VkDescriptorPool              descriptorPools[DSETS];
    std::vector<VkDescriptorSet>  descriptorSets[DSETS];
    std::vector<VkDescriptorSetLayoutBinding> descriptorBindings[DSETS];

    void initSetLayout(VkDevice device, uint32_t dset, VkDescriptorSetLayoutCreateFlags flags = 0, const VkAllocationCallbacks* pAllocator = nullptr)
    {
      DeviceUtils utils(device, pAllocator);
      descriptorSetLayout[dset] = utils.createDescriptorSetLayout((uint32_t)descriptorBindings[dset].size(), descriptorBindings[dset].data(), flags);
    }
    
    void initPoolAndSets(VkDevice device, uint32_t maxSets, size_t poolSizeCount, const VkDescriptorPoolSize* poolSizes, uint32_t dset=0, const VkAllocationCallbacks* pAllocator = nullptr)
    {
      DeviceUtils utils(device,pAllocator);
      descriptorSets[dset].resize(maxSets);
      utils.createDescriptorPoolAndSets(maxSets, poolSizeCount, poolSizes, descriptorSetLayout[dset], descriptorPools[dset], descriptorSets[dset].data());
    }

    void initPoolAndSets(VkDevice device, uint32_t maxSets, uint32_t dset = 0, const VkAllocationCallbacks* pAllocator = nullptr)
    {
      assert(!descriptorBindings[dset].empty());

      DeviceUtils utils(device, pAllocator);
      descriptorSets[dset].resize(maxSets);

      // setup poolsizes for each descriptorType
      std::vector<VkDescriptorPoolSize> poolSizes;
      for (auto it = descriptorBindings[dset].cbegin(); it != descriptorBindings[dset].cend(); ++it) {
        bool found = false;
        for (auto itpool = poolSizes.begin(); itpool != poolSizes.end(); ++itpool) {
          if (itpool->type == it->descriptorType) {
            itpool->descriptorCount += it->descriptorCount;
            found = true;
            break;
          }
        }
        if (!found) {
          VkDescriptorPoolSize poolSize;
          poolSize.type = it->descriptorType;
          poolSize.descriptorCount = it->descriptorCount;
          poolSizes.push_back(poolSize);
        }
      }
      utils.createDescriptorPoolAndSets(maxSets, (uint32_t)poolSizes.size(), poolSizes.data(), descriptorSetLayout[dset], descriptorPools[dset], descriptorSets[dset].data());
    }

    void deinitPools(VkDevice device, const VkAllocationCallbacks* pAllocator = nullptr)
    {
      for (uint32_t i = 0; i < DSETS; i++) {
        if (descriptorPools[i]) {
          vkDestroyDescriptorPool(device, descriptorPools[i], pAllocator);
          descriptorSets[i].clear();
          descriptorPools[i] = nullptr;
        }
      }
    }

    void deinitPool(uint32_t dset, VkDevice device, const VkAllocationCallbacks* pAllocator = nullptr)
    {
      if (descriptorPools[dset]) {
        vkDestroyDescriptorPool(device, descriptorPools[dset], pAllocator);
        descriptorSets[dset].clear();
        descriptorPools[dset] = nullptr;
      }
    }

    void deinitLayouts(VkDevice device, const VkAllocationCallbacks* pAllocator = nullptr)
    {
      for (uint32_t i = 0; i < PIPELAYOUTS; i++) {
        if (pipelineLayouts[i]) {
          vkDestroyPipelineLayout(device, pipelineLayouts[i], pAllocator);
          pipelineLayouts[i] = nullptr;
        }
      }
      for (uint32_t i = 0; i < DSETS; i++) {
        if (descriptorSetLayout[i]) {
          vkDestroyDescriptorSetLayout(device, descriptorSetLayout[i], pAllocator);
          descriptorSetLayout[i] = nullptr;
        }
        descriptorBindings[i].clear();
      }
    }

    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const VkDescriptorImageInfo *pImageInfo) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, pImageInfo);
    }
    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, pBufferInfo);
    }
    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, pTexelBufferView);
    }
    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const void* pNext) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, pNext);
    }

    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorImageInfo *pImageInfo) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, arrayElement, pImageInfo);
    }
    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorBufferInfo* pBufferInfo) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, arrayElement, pBufferInfo);
    }
    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, arrayElement, pTexelBufferView);
    }
    VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, uint32_t arrayElement, const void* pNext) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), descriptorSets[dset][dstSet], dstBinding, arrayElement, pNext);
    }

    VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const VkDescriptorImageInfo *pImageInfo) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pImageInfo);
    }
    VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pBufferInfo);
    }
    VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pTexelBufferView);
    }
    VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const void* pNext) const
    {
      return Makers::makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pNext);
    }

    const VkDescriptorSet* getSets(uint32_t dset=0) const {
      return descriptorSets[dset].data();
    }

    VkPipelineLayout getPipeLayout(uint32_t pipe=0) const {
      return pipelineLayouts[pipe];
    }

    size_t getSetsCount(uint32_t dset=0) const {
      return descriptorSets[dset].size();
    }

    DescriptorPipelineContainer() {
      for (uint32_t i = 0; i < PIPELAYOUTS; i++) {
        pipelineLayouts[i] = nullptr;
      }
      for (uint32_t i = 0; i < DSETS; i++) {
        descriptorSetLayout[i] = nullptr;
        descriptorPools[i] = nullptr;
      }
    }
  };
}

#endif
