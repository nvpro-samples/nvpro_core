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

#include <algorithm>

#include "error_vk.hpp"
#include "memorymanagement_vk.hpp"
#include "nvh/nvprint.hpp"

namespace nvvk {
static VkExportMemoryAllocateInfo memoryHandleEx{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                                 VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT};  // #VKGL Special for Interop

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

const VkMemoryDedicatedAllocateInfo* DeviceMemoryAllocator::DEDICATED_PROXY =
    (const VkMemoryDedicatedAllocateInfo*)&DeviceMemoryAllocator::DEDICATED_PROXY;

int DeviceMemoryAllocator::s_allocDebugBias = 0;

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

  return info.id;
}

void DeviceMemoryAllocator::destroyID(AllocationID id)
{
  assert(m_allocations[id.index].id.isEqual(id));

  // setup for free list
  m_allocations[id.index].id.instantiate(m_freeAllocationIndex);
  m_freeAllocationIndex = id.index;
}

const float DeviceMemoryAllocator::DEFAULT_PRIORITY = 0.5f;

void DeviceMemoryAllocator::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize blockSize, VkDeviceSize maxSize)
{
  m_device            = device;
  m_physicalDevice    = physicalDevice;
  m_blockSize         = blockSize;
  m_maxAllocationSize = maxSize;

  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);

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
    }
  }

  m_allocations.clear();
  m_blocks.clear();
  resizeBlocks(0);

  m_freeBlockIndex      = INVALID_ID_INDEX;
  m_freeAllocationIndex = INVALID_ID_INDEX;
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

void DeviceMemoryAllocator::getTypeStats(uint32_t count[VK_MAX_MEMORY_TYPES], VkDeviceSize used[VK_MAX_MEMORY_TYPES], VkDeviceSize allocated[VK_MAX_MEMORY_TYPES]) const
{
  memset(used, 0, sizeof(used));
  memset(allocated, 0, sizeof(allocated));

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
                                                  bool                                 preferDevice)
{
  VkMemoryAllocateInfo memInfo;

  // Fill out allocation info structure
  if(memReqs.size > m_maxAllocationSize || !getMemoryInfo(m_memoryProperties, memReqs, memProps, memInfo, preferDevice))
  {
    result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
    return AllocationID();
  }

  float priority = m_supportsPriority ? m_priority : DEFAULT_PRIORITY;
  bool  isFirst  = !dedicated;

  if(!dedicated)
  {
    // First try to find an existing memory block that we can use

    for(uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++)
    {
      Block& block = m_blocks[i];

      // Ignore invalid or blocks with the wrong memory type
      if(!block.mem || block.memoryTypeIndex != memInfo.memoryTypeIndex || isLinear != block.isLinear || block.priority != priority)
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
    block.allocationSize = memInfo.allocationSize = memReqs.size;
  }
  else if(dedicated)
  {
    block.allocationSize = memInfo.allocationSize = memReqs.size;
    memInfo.pNext                                 = dedicated;
  }
  else
  {
    block.allocationSize = memInfo.allocationSize = std::max(m_blockSize, memReqs.size);
  }

  VkMemoryPriorityAllocateInfoEXT memPriority = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
  if(priority != DEFAULT_PRIORITY)
  {
    memPriority.pNext    = memInfo.pNext;
    memPriority.priority = priority;
    memInfo.pNext        = &memPriority;
  }

  block.allocationSize  = block.range.alignedSize((uint32_t)block.allocationSize);
  block.priority        = priority;
  block.memoryTypeIndex = memInfo.memoryTypeIndex;
  block.range.init((uint32_t)block.allocationSize);
  block.isLinear    = isLinear;
  block.isFirst     = isFirst;
  block.isDedicated = dedicated != nullptr;

  result = allocBlockMemory(id, memInfo, block.mem);

  if(result == VK_SUCCESS)
  {
    m_allocatedSize += block.allocationSize;

    uint32_t offset;
    uint32_t blockSize;
    uint32_t blockOffset;

    block.range.subAllocate((uint32_t)memReqs.size, (uint32_t)memReqs.alignment, blockOffset, offset, blockSize);

    block.allocationCount = 1;
    block.usedSize        = blockSize;
    block.mapCount        = 0;
    block.mapped          = nullptr;
    block.mappable        = (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

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
      return allocInternal(memReqs, 0, isLinear, dedicated, result, !preferDevice);
    }
    else
    {
      LOGE("could not allocate memory: VkResult %d\n", result);
      assert(0);
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

void* DeviceMemoryAllocator::map(AllocationID allocationID)
{
  const AllocationInfo& info  = getInfo(allocationID);
  Block&                block = getBlock(info.block);

  assert(block.mappable);
  block.mapCount++;

  if(!block.mapped)
  {
    VkResult result = vkMapMemory(m_device, block.mem, 0, block.allocationSize, 0, (void**)&block.mapped);
    assert(result == VK_SUCCESS);
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

  result = vkCreateImage(m_device, &createInfo, nullptr, &image);
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

  result = vkCreateBuffer(m_device, &createInfo, nullptr, &buffer);
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

//////////////////////////////////////////////////////////////////////////

void StagingMemoryManager::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize /*= 64 * 1024 * 1024*/)
{
  m_device           = device;
  m_physicalDevice   = physicalDevice;
  m_stagingBlockSize = stagingBlockSize;

  assert(m_sets.empty());
  assert(m_blocks.empty());
}

void StagingMemoryManager::deinit()
{
  free(false);

  m_sets.clear();
  m_freeStagingIndex = INVALID_ID_INDEX;
}

bool StagingMemoryManager::fitsInAllocated(VkDeviceSize size) const
{
  for(const auto& block : m_blocks)
  {
    if(block.buffer)
    {
      if(block.range.isAvailable((uint32_t)size, 16))
      {
        return true;
      }
    }
  }

  return false;
}

void* StagingMemoryManager::cmdToImage(VkCommandBuffer                 cmd,
                                       VkImage                         image,
                                       const VkOffset3D&               offset,
                                       const VkExtent3D&               extent,
                                       const VkImageSubresourceLayers& subresource,
                                       VkDeviceSize                    size,
                                       const void*                     data)
{
  VkBuffer     srcBuffer;
  VkDeviceSize srcOffset;

  void* mapping = getStagingSpace(size, srcBuffer, srcOffset);

  assert(mapping);

  if(data)
  {
    memcpy(mapping, data, size);
  }

  VkBufferImageCopy cpy;
  cpy.bufferOffset      = srcOffset;
  cpy.bufferRowLength   = 0;
  cpy.bufferImageHeight = 0;
  cpy.imageSubresource  = subresource;
  cpy.imageOffset       = offset;
  cpy.imageExtent       = extent;

  vkCmdCopyBufferToImage(cmd, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);

  return data ? nullptr : mapping;
}

void* StagingMemoryManager::cmdToBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
  if(!size)
  {
    return nullptr;
  }

  VkBuffer     srcBuffer;
  VkDeviceSize srcOffset;

  void* mapping = getStagingSpace(size, srcBuffer, srcOffset);

  assert(mapping);

  if(data)
  {
    memcpy(mapping, data, size);
  }

  VkBufferCopy cpy;
  cpy.size      = size;
  cpy.srcOffset = srcOffset;
  cpy.dstOffset = offset;

  vkCmdCopyBuffer(cmd, srcBuffer, buffer, 1, &cpy);

  return data ? nullptr : (void*)mapping;
}

StagingID StagingMemoryManager::finalizeCmds(VkFence fence)
{
  if(!m_current.isValid())
  {
    m_current = createID();
  }

  StagingID current             = m_current;
  m_sets[m_current.index].fence = fence;

  m_current.invalidate();

  return current;
}


void* StagingMemoryManager::getStagingSpace(VkDeviceSize size, VkBuffer& buffer, VkDeviceSize& offset)
{
  if(!m_current.isValid())
  {
    m_current = createID();
  }

  uint32_t usedOffset;
  uint32_t usedSize;
  uint32_t usedAligned;

  BlockID id;

  for(uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++)
  {
    Block& block = m_blocks[i];
    if(block.buffer && block.range.subAllocate((uint32_t)size, 16, usedOffset, usedAligned, usedSize))
    {
      id = block.id;

      offset = usedAligned;
      buffer = block.buffer;
    }
  }

  if(id.index == INVALID_ID_INDEX)
  {
    if(m_freeBlockIndex != INVALID_ID_INDEX)
    {
      Block& block     = m_blocks[m_freeBlockIndex];
      m_freeBlockIndex = block.id.instantiate(m_freeBlockIndex);

      id = block.id;
    }
    else
    {
      uint32_t newIndex = (uint32_t)m_blocks.size();
      m_blocks.resize(m_blocks.size() + 1);
      resizeBlocks((uint32_t)m_blocks.size());
      Block& block = m_blocks[newIndex];
      block.id.instantiate(newIndex);

      id = block.id;
    }

    Block& block = m_blocks[id.index];
    block.size   = std::max(m_stagingBlockSize, size);
    block.size   = block.range.alignedSize((uint32_t)block.size);

    VkResult result = allocBlockMemory(id, block.size, true, block);
    NVVK_CHECK(result);

    m_allocatedSize += block.size;

    block.range.init((uint32_t)block.size);
    block.range.subAllocate((uint32_t)size, 16, usedOffset, usedAligned, usedSize);

    offset = usedAligned;
    buffer = block.buffer;
  }

  // append used space to current staging set list
  m_usedSize += usedSize;
  m_sets[m_current.index].entries.push_back({id, usedOffset, usedSize});

  return m_blocks[id.index].mapping + offset;
}

void StagingMemoryManager::release(StagingID stagingID)
{
  StagingSet& set = m_sets[stagingID.index];
  assert(set.id.isEqual(stagingID));

  // free used allocation ranges
  for(auto& itentry : set.entries)
  {
    Block& block = getBlock(itentry.block);
    block.range.subFree(itentry.offset, itentry.size);

    m_usedSize -= itentry.size;

    if(block.range.isEmpty() && m_freeOnRelease)
    {
      freeBlock(block);
    }
  }
  set.entries.clear();
  set.fence = VK_NULL_HANDLE;

  // setup free-list
  m_freeStagingIndex = set.id.instantiate(m_freeStagingIndex);
}

void StagingMemoryManager::tryReleaseFenced()
{
  for(auto& itset : m_sets)
  {
    if(itset.fence)
    {
      VkResult result = vkGetFenceStatus(m_device, itset.fence);
      if(result == VK_SUCCESS)
      {
        release(itset.id);
      }
    }
  }
}

float StagingMemoryManager::getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const
{
  allocatedSize = m_allocatedSize;
  usedSize      = m_usedSize;

  return float(double(usedSize) / double(allocatedSize));
}

void StagingMemoryManager::free(bool unusedOnly)
{
  for(uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++)
  {
    Block& block = m_blocks[i];
    if(block.buffer && (block.range.isEmpty() || !unusedOnly))
    {
      freeBlock(block);
    }
  }

  if(!unusedOnly)
  {
    m_blocks.clear();
    resizeBlocks(0);
    m_freeBlockIndex = INVALID_ID_INDEX;
  }
}

void StagingMemoryManager::freeBlock(Block& block)
{
  m_allocatedSize -= block.size;
  freeBlockMemory(block.id, block);
  block.memory  = VK_NULL_HANDLE;
  block.buffer  = VK_NULL_HANDLE;
  block.mapping = nullptr;
  block.range.deinit();
  m_freeBlockIndex = block.id.instantiate(m_freeBlockIndex);
}

StagingID StagingMemoryManager::createID()
{
  // find free slot
  if(m_freeStagingIndex != INVALID_ID_INDEX)
  {
    uint32_t newIndex  = m_freeStagingIndex;
    m_freeStagingIndex = m_sets[newIndex].id.instantiate(newIndex);
    return m_sets[newIndex].id;
  }

  // otherwise push to end
  StagingSet info;
  info.id.instantiate((uint32_t)m_sets.size());

  m_sets.push_back(info);

  return info.id;
}

VkResult StagingMemoryManager::allocBlockMemory(BlockID id, VkDeviceSize size, bool toDevice, Block& block)
{
  VkResult           result;
  VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size               = size;
  createInfo.usage              = toDevice ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VkBuffer buffer = VK_NULL_HANDLE;
  result          = vkCreateBuffer(m_device, &createInfo, nullptr, &buffer);
  if(result != VK_SUCCESS)
  {
    NVVK_CHECK(result);
    return result;
  }

  VkMemoryRequirements2           memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements   dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkBufferMemoryRequirementsInfo2 bufferReqs    = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};

  bufferReqs.buffer = buffer;
  memReqs.pNext     = &dedicatedRegs;
  vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);

  VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
  dedicatedInfo.buffer                        = buffer;

  if(m_memoryTypeIndex == ~0)
  {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                     | (toDevice ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

    // Find an available memory type that satisfies the requested properties.
    for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
      if((memReqs.memoryRequirements.memoryTypeBits & (1 << memoryTypeIndex))
         && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps)
      {
        m_memoryTypeIndex = memoryTypeIndex;
        break;
      }
    }
  }

  if(m_memoryTypeIndex == ~0)
  {
    LOGE("could not find memoryTypeIndex\n");
    vkDestroyBuffer(m_device, buffer, nullptr);
    return VK_ERROR_INCOMPATIBLE_DRIVER;
  }

  VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memInfo.allocationSize       = size;
  memInfo.memoryTypeIndex      = m_memoryTypeIndex;
  memInfo.pNext                = &dedicatedInfo;

  VkDeviceMemory mem = VK_NULL_HANDLE;
  result             = vkAllocateMemory(m_device, &memInfo, nullptr, &mem);
  if(result != VK_SUCCESS)
  {
    NVVK_CHECK(result);
    vkDestroyBuffer(m_device, buffer, nullptr);
    return result;
  }

  VkBindBufferMemoryInfo bindInfos = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
  bindInfos.buffer                 = buffer;
  bindInfos.memory                 = mem;

  result = vkBindBufferMemory2(m_device, 1, &bindInfos);

  if(result == VK_SUCCESS)
  {
    result = vkMapMemory(m_device, mem, 0, size, 0, (void**)&block.mapping);
    if(result == VK_SUCCESS)
    {
      block.memory = mem;
      block.buffer = buffer;
      return result;
    }
  }
  // error case
  NVVK_CHECK(result);
  vkDestroyBuffer(m_device, buffer, nullptr);
  vkFreeMemory(m_device, mem, nullptr);
  return result;
}

void StagingMemoryManager::freeBlockMemory(BlockID id, const Block& block)
{
  vkDestroyBuffer(m_device, block.buffer, nullptr);
  vkUnmapMemory(m_device, block.memory);
  vkFreeMemory(m_device, block.memory, nullptr);
}


}  // namespace nvvk
