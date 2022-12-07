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


#include <algorithm>
#include <string>

#include "debug_util_vk.hpp"
#include "error_vk.hpp"
#include "memorymanagement_vk.hpp"
#include "nvh/nvprint.hpp"

namespace nvvk {
bool getMemoryInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties,
                   const VkMemoryRequirements&             memReqs,
                   VkMemoryPropertyFlags                   properties,
                   VkMemoryAllocateInfo&                   memInfo,
                   bool                                    preferDevice)
{
  memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memInfo.pNext = nullptr;

  if(!memReqs.size)
  {
    memInfo.allocationSize  = 0;
    memInfo.memoryTypeIndex = ~0;
    return true;
  }

  // Find an available memory type that satisfies the requested properties.
  for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
  {
    if((memReqs.memoryTypeBits & (1 << memoryTypeIndex))
       // either there is a propertyFlags that also includes the combinations
       && ((properties && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties)
           // or it directly matches the properties (zero case)
           || (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags == properties)))
    {
      memInfo.allocationSize  = memReqs.size;
      memInfo.memoryTypeIndex = memoryTypeIndex;
      return true;
    }
  }

  // special case zero flag logic
  if(properties == 0)
  {
    // prefer something with host visible
    return getMemoryInfo(memoryProperties, memReqs,
                         preferDevice ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memInfo);
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

class DMAMemoryAllocator;

class DMAMemoryHandle : public nvvk::MemHandleBase
{
public:
  DMAMemoryHandle()                       = default;
  DMAMemoryHandle(const DMAMemoryHandle&) = default;
  DMAMemoryHandle(DMAMemoryHandle&&)      = default;

  DMAMemoryHandle& operator=(const DMAMemoryHandle&) = default;
  DMAMemoryHandle& operator=(DMAMemoryHandle&&) = default;

  const AllocationID& getAllocationID() const { return m_allocation; };

private:
  friend class nvvk::DeviceMemoryAllocator;
  DMAMemoryHandle(const AllocationID& allocation)
      : m_allocation(allocation)
  {
  }

  AllocationID m_allocation;
};

DMAMemoryHandle* castDMAMemoryHandle(MemHandle memHandle)
{
  if(!memHandle)
    return nullptr;
#ifndef NDEBUG
  auto dmaMemHandle = static_cast<DMAMemoryHandle*>(memHandle);
#else
  auto dmaMemHandle = dynamic_cast<DMAMemoryHandle*>(memHandle);
  assert(dmaMemHandle);
#endif

  return dmaMemHandle;
}

MemHandle DeviceMemoryAllocator::allocMemory(const MemAllocateInfo& allocInfo, VkResult *pResult)
{
  BakedAllocateInfo bakedInfo;
  fillBakedAllocateInfo(getMemoryProperties(), allocInfo, bakedInfo);
  State state = m_defaultState;
  state.allocateDeviceMask |= bakedInfo.flagsInfo.deviceMask;
  state.allocateFlags |= bakedInfo.flagsInfo.flags;
  state.priority = allocInfo.getPriority();

  VkResult result;
  bool isDedicatedAllocation = allocInfo.getDedicatedBuffer() || allocInfo.getDedicatedImage();

  auto dmaHandle = allocInternal(allocInfo.getMemoryRequirements(), allocInfo.getMemoryProperties(),
                                !allocInfo.getTilingOptimal() /*isLinear*/,
                                isDedicatedAllocation ? &bakedInfo.dedicatedInfo : nullptr, result, true, state);

  if (pResult)
  {
    *pResult = result;
  }

  if (dmaHandle)
  {
    DMAMemoryHandle* dmaMemHandle = new DMAMemoryHandle(dmaHandle);

    // Cannot do this, it would override the DeviceMemoryManager's chosen block buffer name
    //   if(!allocInfo.getDebugName().empty())
    //   {
    //     const MemInfo& memInfo = getMemoryInfo(dmaMemHandle);
    //     nvvk::DebugUtil(m_dma.getDevice()).setObjectName(memInfo.memory, allocInfo.getDebugName());
    //   }

    return dmaMemHandle;
  }
  else
  {
    return NullMemHandle;
  }
}

void DeviceMemoryAllocator::freeMemory(MemHandle memHandle)
{
  if(!memHandle)
    return;

  auto dmaHandle = castDMAMemoryHandle(memHandle);
  assert(dmaHandle);

  free(dmaHandle->getAllocationID());

  delete dmaHandle;

  return;
}

MemAllocator::MemInfo DeviceMemoryAllocator::getMemoryInfo(MemHandle memHandle) const
{
  MemInfo info;

  auto dmaHandle = castDMAMemoryHandle(memHandle);
  assert(dmaHandle);

  auto& allocInfo = getAllocation(dmaHandle->getAllocationID());
  info.memory     = allocInfo.mem;
  info.offset     = allocInfo.offset;
  info.size       = allocInfo.size;

  return info;
};

nvvk::AllocationID DeviceMemoryAllocator::getAllocationID(MemHandle memHandle) const
{
  auto dmaHandle = castDMAMemoryHandle(memHandle);
  assert(dmaHandle);

  return dmaHandle->getAllocationID();
}


void* DeviceMemoryAllocator::map(MemHandle memHandle, VkDeviceSize offset, VkDeviceSize size, VkResult *pResult)
{
  auto dmaHandle = castDMAMemoryHandle(memHandle);
  assert(dmaHandle);

  void* ptr = map(dmaHandle->getAllocationID(), pResult);
  return ptr;
}

void DeviceMemoryAllocator::unmap(MemHandle memHandle)
{
  auto dmaHandle = castDMAMemoryHandle(memHandle);
  assert(dmaHandle);

  unmap(dmaHandle->getAllocationID());
}

const VkMemoryDedicatedAllocateInfo* DeviceMemoryAllocator::DEDICATED_PROXY =
    (const VkMemoryDedicatedAllocateInfo*)&DeviceMemoryAllocator::DEDICATED_PROXY;

int DeviceMemoryAllocator::s_allocDebugBias = 0;

//#define DEBUG_ALLOCID   8

nvvk::AllocationID DeviceMemoryAllocator::createID(Allocation& allocation, BlockID block, uint32_t blockOffset, uint32_t blockSize)
{
  // find free slot
  if(m_freeAllocationIndex != INVALID_ID_INDEX)
  {
    uint32_t index = m_freeAllocationIndex;

    m_freeAllocationIndex            = m_allocations[index].id.instantiate((uint32_t)index);
    m_allocations[index].allocation  = allocation;
    m_allocations[index].block       = block;
    m_allocations[index].blockOffset = blockOffset;
    m_allocations[index].blockSize   = blockSize;
#if DEBUG_ALLOCID
    // debug some specific id, useful to track allocation leaks
    if(index == DEBUG_ALLOCID)
    {
      int breakHere = 0;
      breakHere     = breakHere;
    }
#endif
    return m_allocations[index].id;
  }

  // otherwise push to end
  AllocationInfo info;
  info.allocation = allocation;
  info.id.instantiate((uint32_t)m_allocations.size());
  info.block       = block;
  info.blockOffset = blockOffset;
  info.blockSize   = blockSize;

  m_allocations.push_back(info);

#if DEBUG_ALLOCID
  // debug some specific id, useful to track allocation leaks
  if(info.id.index == DEBUG_ALLOCID)
  {
    int breakHere = 0;
    breakHere     = breakHere;
  }
#endif

  return info.id;
}

void DeviceMemoryAllocator::destroyID(AllocationID id)
{
  assert(m_allocations[id.index].id.isEqual(id));

#if DEBUG_ALLOCID
  // debug some specific id, useful to track allocation leaks
  if(id.index == DEBUG_ALLOCID)
  {
    int breakHere = 0;
    breakHere     = breakHere;
  }
#endif

  // setup for free list
  m_allocations[id.index].id.instantiate(m_freeAllocationIndex);
  m_freeAllocationIndex = id.index;
}

const float DeviceMemoryAllocator::DEFAULT_PRIORITY = 0.5f;

void DeviceMemoryAllocator::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize blockSize, VkDeviceSize maxSize)
{
  assert(!m_device);
  m_device         = device;
  m_physicalDevice = physicalDevice;
  // always default to NVVK_DEFAULT_MEMORY_BLOCKSIZE
  m_blockSize      = blockSize ? blockSize : NVVK_DEFAULT_MEMORY_BLOCKSIZE;

  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);

  // Retrieving the max allocation size, can be lowered with maxSize
  VkPhysicalDeviceProperties2            prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  VkPhysicalDeviceMaintenance3Properties vkProp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES};
  prop2.pNext = &vkProp;
  vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);
  m_maxAllocationSize = maxSize > 0 ? std::min(maxSize, vkProp.maxMemoryAllocationSize) : vkProp.maxMemoryAllocationSize;


  assert(m_blocks.empty());
  assert(m_allocations.empty());
}

void DeviceMemoryAllocator::freeAll()
{
  for(const auto& it : m_blocks)
  {
    if(!it.mem)
      continue;

    if(it.mapped)
    {
      vkUnmapMemory(m_device, it.mem);
    }
    vkFreeMemory(m_device, it.mem, nullptr);
  }

  m_allocations.clear();
  m_blocks.clear();
  resizeBlocks(0);

  m_freeBlockIndex      = INVALID_ID_INDEX;
  m_freeAllocationIndex = INVALID_ID_INDEX;
}

void DeviceMemoryAllocator::deinit()
{
  if(!m_device)
    return;

  for(const auto& it : m_blocks)
  {
    if(it.mapped)
    {
      assert("not all blocks were unmapped properly");
      if(it.mem)
      {
        vkUnmapMemory(m_device, it.mem);
      }
    }
    if(it.mem)
    {
      if(it.isFirst && m_keepFirst)
      {
        vkFreeMemory(m_device, it.mem, nullptr);
      }
      else
      {
        assert("not all blocks were freed properly");
      }
    }
  }

  for(size_t i = 0; i < m_allocations.size(); i++)
  {
    if(m_allocations[i].id.index == (uint32_t)i)
    {
      assert(0 && i && "AllocationID not freed");

      // set DEBUG_ALLOCID define further up to trace this id
    }
  }

  m_allocations.clear();
  m_blocks.clear();
  resizeBlocks(0);

  m_freeBlockIndex      = INVALID_ID_INDEX;
  m_freeAllocationIndex = INVALID_ID_INDEX;
  m_device              = VK_NULL_HANDLE;
}

VkDeviceSize DeviceMemoryAllocator::getMaxAllocationSize() const
{
  return m_maxAllocationSize;
}

float DeviceMemoryAllocator::getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const
{
  allocatedSize = m_allocatedSize;
  usedSize      = m_usedSize;

  return float(double(usedSize) / double(allocatedSize));
}

void DeviceMemoryAllocator::nvprintReport() const
{
  VkDeviceSize used[VK_MAX_MEMORY_HEAPS]      = {0};
  VkDeviceSize allocated[VK_MAX_MEMORY_HEAPS] = {0};
  uint32_t     active[VK_MAX_MEMORY_HEAPS]    = {0};
  uint32_t     dedicated[VK_MAX_MEMORY_HEAPS] = {0};
  uint32_t     linear[VK_MAX_MEMORY_HEAPS]    = {0};

  uint32_t dedicatedSum = 0;
  uint32_t linearSum    = 0;
  for(const auto& block : m_blocks)
  {
    if(block.mem)
    {
      uint32_t heapIndex = m_memoryProperties.memoryTypes[block.memoryTypeIndex].heapIndex;
      used[heapIndex] += block.usedSize;
      allocated[heapIndex] += block.allocationSize;

      active[heapIndex]++;
      linear[heapIndex] += block.isLinear ? 1 : 0;
      dedicated[heapIndex] += block.isDedicated ? 1 : 0;

      linearSum += block.isLinear ? 1 : 0;
      dedicatedSum += block.isDedicated ? 1 : 0;
    }
  }

  LOGI("nvvk::DeviceMemoryAllocator %p\n", this);
  {
    LOGI("  count : dedicated, linear,  all (device-local)\n");
  }
  for(uint32_t i = 0; i < m_memoryProperties.memoryHeapCount; i++)
  {
    LOGI("  heap%d : %9d, %6d, %4d (%d)\n", i, dedicated[i], linear[i], active[i],
         (m_memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? 1 : 0);
  }

  {
    LOGI("  total : %9d, %6d, %4d\n", dedicatedSum, linearSum, m_activeBlockCount);
    LOGI("  size  :      used / allocated / available KB (device-local)\n");
  }
  for(uint32_t i = 0; i < m_memoryProperties.memoryHeapCount; i++)
  {
    LOGI("  heap%d : %9d / %9d / %9d (%d)\n", i, uint32_t((used[i] + 1023) / 1024),
         uint32_t((allocated[i] + 1023) / 1024), uint32_t((m_memoryProperties.memoryHeaps[i].size + 1023) / 1024),
         (m_memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? 1 : 0);
  }
  {
    LOGI("  total : %9d / %9d KB (%d percent)\n\n", uint32_t((m_usedSize + 1023) / 1024),
         uint32_t((m_allocatedSize + 1023) / 1024), uint32_t(double(m_usedSize) * 100.0 / double(m_allocatedSize)));
  }
}

void DeviceMemoryAllocator::getTypeStats(uint32_t     count[VK_MAX_MEMORY_TYPES],
                                         VkDeviceSize used[VK_MAX_MEMORY_TYPES],
                                         VkDeviceSize allocated[VK_MAX_MEMORY_TYPES]) const
{
  memset(used, 0, sizeof(used[0]) * VK_MAX_MEMORY_TYPES);
  memset(allocated, 0, sizeof(allocated[0]) * VK_MAX_MEMORY_TYPES);

  for(const auto& block : m_blocks)
  {
    if(block.mem)
    {
      count[block.memoryTypeIndex]++;
      used[block.memoryTypeIndex] += block.usedSize;
      allocated[block.memoryTypeIndex] += block.allocationSize;
    }
  }
}

VkDevice DeviceMemoryAllocator::getDevice() const
{
  return m_device;
}

VkPhysicalDevice DeviceMemoryAllocator::getPhysicalDevice() const
{
  return m_physicalDevice;
}

const nvvk::Allocation& DeviceMemoryAllocator::getAllocation(AllocationID id) const
{
  assert(m_allocations[id.index].id.isEqual(id));
  return m_allocations[id.index].allocation;
}

const VkPhysicalDeviceMemoryProperties& DeviceMemoryAllocator::getMemoryProperties() const
{
  return m_memoryProperties;
}

AllocationID DeviceMemoryAllocator::allocInternal(const VkMemoryRequirements&          memReqs,
                                                  VkMemoryPropertyFlags                memProps,
                                                  bool                                 isLinear,
                                                  const VkMemoryDedicatedAllocateInfo* dedicated,
                                                  VkResult&                            result,
                                                  bool                                 preferDevice,
                                                  const State&                         state)
{
  VkMemoryAllocateInfo memInfo;

  result = VK_SUCCESS;

  // Fill out allocation info structure
  if(memReqs.size > m_maxAllocationSize || !nvvk::getMemoryInfo(m_memoryProperties, memReqs, memProps, memInfo, preferDevice))
  {
    result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    return AllocationID();
  }

  float priority = m_supportsPriority ? state.priority : DEFAULT_PRIORITY;
  bool  isFirst  = !dedicated;
  bool  mappable = (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

  if(!dedicated)
  {
    // First try to find an existing memory block that we can use

    for(uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++)
    {
      Block& block = m_blocks[i];

      // Ignore invalid or blocks with the wrong memory type
      if(!block.mem || block.memoryTypeIndex != memInfo.memoryTypeIndex || isLinear != block.isLinear || block.priority != priority
         || block.allocateFlags != state.allocateFlags || block.allocateDeviceMask != state.allocateDeviceMask
         || (!block.mappable && mappable))
      {
        continue;
      }

      // if there is a compatible block, we are not "first" of a kind
      isFirst = false;

      uint32_t blockSize;
      uint32_t blockOffset;
      uint32_t offset;


      // Look for a block which has enough free space available

      if(block.range.subAllocate((uint32_t)memReqs.size, (uint32_t)memReqs.alignment, blockOffset, offset, blockSize))
      {
        block.allocationCount++;
        block.usedSize += blockSize;

        Allocation allocation;
        allocation.mem    = block.mem;
        allocation.offset = offset;
        allocation.size   = memReqs.size;

        m_usedSize += blockSize;

        return createID(allocation, block.id, blockOffset, blockSize);
      }
    }
  }

  // find available blockID or create new one
  BlockID id;
  if(m_freeBlockIndex != INVALID_ID_INDEX)
  {
    Block& block     = m_blocks[m_freeBlockIndex];
    m_freeBlockIndex = block.id.instantiate(m_freeBlockIndex);
    id               = block.id;
  }
  else
  {
    uint32_t newIndex = (uint32_t)m_blocks.size();
    m_blocks.resize(m_blocks.size() + 1);
    resizeBlocks(newIndex + 1);
    Block& block = m_blocks[newIndex];
    block.id.instantiate(newIndex);
    id = block.id;
  }

  Block& block = m_blocks[id.index];

  // enforce custom block under certain conditions
  if(dedicated == DEDICATED_PROXY || memReqs.size > ((m_blockSize * 2) / 3))
  {
    block.allocationSize = memReqs.size;
  }
  else if(dedicated)
  {
    block.allocationSize = memReqs.size;
    memInfo.pNext        = dedicated;
  }
  else
  {
    block.allocationSize = std::max(m_blockSize, memReqs.size);
  }

  VkMemoryPriorityAllocateInfoEXT memPriority = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
  if(priority != DEFAULT_PRIORITY)
  {
    memPriority.pNext    = memInfo.pNext;
    memPriority.priority = priority;
    memInfo.pNext        = &memPriority;
  }

  VkMemoryAllocateFlagsInfo memFlags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
  if(state.allocateFlags)
  {
    memFlags.pNext      = memInfo.pNext;
    memFlags.deviceMask = state.allocateDeviceMask;
    memFlags.flags      = state.allocateFlags;
    memInfo.pNext       = &memFlags;
  }

  block.allocationSize  = block.range.alignedSize((uint32_t)block.allocationSize);
  block.priority        = priority;
  block.memoryTypeIndex = memInfo.memoryTypeIndex;
  block.range.init((uint32_t)block.allocationSize);
  block.isLinear           = isLinear;
  block.isFirst            = isFirst;
  block.isDedicated        = dedicated != nullptr;
  block.allocateFlags      = state.allocateFlags;
  block.allocateDeviceMask = state.allocateDeviceMask;

  // set allocationSize from aligned block.allocationSize
  memInfo.allocationSize = block.allocationSize;

  result = allocBlockMemory(id, memInfo, block.mem);

  if(result == VK_SUCCESS)
  {
    nvvk::DebugUtil(m_device).setObjectName(block.mem, m_debugName);

    m_allocatedSize += block.allocationSize;

    uint32_t offset;
    uint32_t blockSize;
    uint32_t blockOffset;

    block.range.subAllocate((uint32_t)memReqs.size, (uint32_t)memReqs.alignment, blockOffset, offset, blockSize);

    block.allocationCount = 1;
    block.usedSize        = blockSize;
    block.mapCount        = 0;
    block.mapped          = nullptr;
    block.mappable        = mappable;

    Allocation allocation;
    allocation.mem    = block.mem;
    allocation.offset = offset;
    allocation.size   = memReqs.size;

    m_usedSize += blockSize;

    m_activeBlockCount++;

    return createID(allocation, id, blockOffset, blockSize);
  }
  else
  {
    // make block free
    m_freeBlockIndex = block.id.instantiate(m_freeBlockIndex);

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY
       && ((memProps == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) || (memProps == 0 && preferDevice)))
    {
      // downgrade memory property to zero and/or not preferDevice
      LOGW("downgrade memory\n");
      return allocInternal(memReqs, 0, isLinear, dedicated, result, !preferDevice, state);
    }
    else
    {
      LOGE("could not allocate memory: VkResult %d\n", result);
      return AllocationID();
    }
  }
}

void DeviceMemoryAllocator::free(AllocationID allocationID)
{
  const AllocationInfo& info  = getInfo(allocationID);
  Block&                block = getBlock(info.block);

  destroyID(allocationID);

  m_usedSize -= info.blockSize;
  block.range.subFree(info.blockOffset, info.blockSize);
  block.allocationCount--;
  block.usedSize -= info.blockSize;

  if(block.allocationCount == 0 && !(block.isFirst && m_keepFirst))
  {
    assert(block.usedSize == 0);
    assert(!block.mapped);
    freeBlockMemory(info.block, block.mem);
    block.mem     = VK_NULL_HANDLE;
    block.isFirst = false;

    m_allocatedSize -= block.allocationSize;
    block.range.deinit();

    m_freeBlockIndex = block.id.instantiate(m_freeBlockIndex);
    m_activeBlockCount--;
  }
}

void* DeviceMemoryAllocator::map(AllocationID allocationID, VkResult *pResult)
{
  const AllocationInfo& info  = getInfo(allocationID);
  Block&                block = getBlock(info.block);

  assert(block.mappable);
  block.mapCount++;

  if(!block.mapped)
  {
    VkResult result = vkMapMemory(m_device, block.mem, 0, block.allocationSize, 0, (void**)&block.mapped);
    if (pResult)
    {
      *pResult = result;
    }
  }
  return block.mapped + info.allocation.offset;
}

void DeviceMemoryAllocator::unmap(AllocationID allocationID)
{
  const AllocationInfo& info  = getInfo(allocationID);
  Block&                block = getBlock(info.block);

  assert(block.mapped);

  if(--block.mapCount == 0)
  {
    block.mapped = nullptr;
    vkUnmapMemory(m_device, block.mem);
  }
}

VkImage DeviceMemoryAllocator::createImage(const VkImageCreateInfo& createInfo,
                                           AllocationID&            allocationID,
                                           VkMemoryPropertyFlags    memProps,
                                           VkResult&                result)
{
  VkImage image;

  assert(createInfo.extent.width && createInfo.extent.height && createInfo.extent.depth);

  result = createImageInternal(m_device, &createInfo, &image);
  if(result != VK_SUCCESS)
    return VK_NULL_HANDLE;

  VkMemoryRequirements2          memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements  dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkImageMemoryRequirementsInfo2 imageReqs     = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};

  imageReqs.image = image;
  memReqs.pNext   = &dedicatedRegs;
  vkGetImageMemoryRequirements2(m_device, &imageReqs, &memReqs);

  VkBool32 useDedicated = m_forceDedicatedAllocation || dedicatedRegs.prefersDedicatedAllocation;

  VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
  dedicatedInfo.image                         = image;

  allocationID          = alloc(memReqs.memoryRequirements, memProps, createInfo.tiling == VK_IMAGE_TILING_LINEAR,
                       useDedicated ? &dedicatedInfo : nullptr);
  Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

  if(allocation.mem == VK_NULL_HANDLE)
  {
    vkDestroyImage(m_device, image, nullptr);
    result = VK_ERROR_OUT_OF_POOL_MEMORY;
    return VK_NULL_HANDLE;
  }

  VkBindImageMemoryInfo bindInfos = {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
  bindInfos.image                 = image;
  bindInfos.memory                = allocation.mem;
  bindInfos.memoryOffset          = allocation.offset;

  result = vkBindImageMemory2(m_device, 1, &bindInfos);
  if(result != VK_SUCCESS)
  {
    vkDestroyImage(m_device, image, nullptr);
    return VK_NULL_HANDLE;
  }

  return image;
}
VkBuffer DeviceMemoryAllocator::createBuffer(const VkBufferCreateInfo& createInfo,
                                             AllocationID&             allocationID,
                                             VkMemoryPropertyFlags     memProps,
                                             VkResult&                 result)
{
  VkBuffer buffer;

  assert(createInfo.size);

  result = createBufferInternal(m_device, &createInfo, &buffer);
  if(result != VK_SUCCESS)
  {
    return VK_NULL_HANDLE;
  }

  VkMemoryRequirements2           memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements   dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkBufferMemoryRequirementsInfo2 bufferReqs    = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};

  bufferReqs.buffer = buffer;
  memReqs.pNext     = &dedicatedRegs;
  vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);

  // for buffers don't use "preferred", but only requires
  VkBool32 useDedicated = m_forceDedicatedAllocation || dedicatedRegs.requiresDedicatedAllocation;

  VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
  dedicatedInfo.buffer                        = buffer;

  allocationID          = alloc(memReqs.memoryRequirements, memProps, true, useDedicated ? &dedicatedInfo : nullptr);
  Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

  if(allocation.mem == VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, buffer, nullptr);
    result = VK_ERROR_OUT_OF_POOL_MEMORY;
    return VK_NULL_HANDLE;
  }

  VkBindBufferMemoryInfo bindInfos = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
  bindInfos.buffer                 = buffer;
  bindInfos.memory                 = allocation.mem;
  bindInfos.memoryOffset           = allocation.offset;

  result = vkBindBufferMemory2(m_device, 1, &bindInfos);
  if(result != VK_SUCCESS)
  {
    vkDestroyBuffer(m_device, buffer, nullptr);
    return VK_NULL_HANDLE;
  }

  return buffer;
}

VkBuffer DeviceMemoryAllocator::createBuffer(VkDeviceSize          size,
                                             VkBufferUsageFlags    usage,
                                             AllocationID&         allocationID,
                                             VkMemoryPropertyFlags memProps,
                                             VkResult&             result)
{
  VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.usage              = usage | m_defaultBufferUsageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  createInfo.size               = size;

  return createBuffer(createInfo, allocationID, memProps, result);
}

#if VK_NV_ray_tracing
VkAccelerationStructureNV DeviceMemoryAllocator::createAccStructure(const VkAccelerationStructureCreateInfoNV& createInfo,
                                                                    AllocationID&         allocationID,
                                                                    VkMemoryPropertyFlags memProps,
                                                                    VkResult&             result)
{
  VkAccelerationStructureNV accel;
  result = vkCreateAccelerationStructureNV(m_device, &createInfo, nullptr, &accel);
  if(result != VK_SUCCESS)
  {
    return VK_NULL_HANDLE;
  }

  VkMemoryRequirements2                           memReqs = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkAccelerationStructureMemoryRequirementsInfoNV memInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
  memInfo.accelerationStructure = accel;
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memInfo, &memReqs);

  allocationID = alloc(memReqs.memoryRequirements, memProps, true, m_forceDedicatedAllocation ? DEDICATED_PROXY : nullptr);

  Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

  if(allocation.mem == VK_NULL_HANDLE)
  {
    vkDestroyAccelerationStructureNV(m_device, accel, nullptr);
    result = VK_ERROR_OUT_OF_POOL_MEMORY;
    return VK_NULL_HANDLE;
  }

  VkBindAccelerationStructureMemoryInfoNV bind = {VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV};
  bind.accelerationStructure                   = accel;
  bind.memory                                  = allocation.mem;
  bind.memoryOffset                            = allocation.offset;

  assert(allocation.offset % memReqs.memoryRequirements.alignment == 0);

  result = vkBindAccelerationStructureMemoryNV(m_device, 1, &bind);
  if(result != VK_SUCCESS)
  {
    vkDestroyAccelerationStructureNV(m_device, accel, nullptr);
    free(allocationID);
    allocationID = AllocationID();
    return VK_NULL_HANDLE;
  }

  return accel;
}
#endif


}  // namespace nvvk
