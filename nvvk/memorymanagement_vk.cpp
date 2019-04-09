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

#include "memorymanagement_vk.hpp"
#include "physical_vk.hpp"


namespace nvvk 
{

  nvvk::AllocationID BlockDeviceMemoryAllocator::createID(Allocation& allocation)
  {
    // find free slot
    for(size_t i = 0; i < m_allocations.size(); i++)
    {
      if(!m_allocations[i].id.isValid())
      {
        m_allocations[i].id.index = (uint32_t)i;
        m_allocations[i].id.incarnation++;
        m_allocations[i].allocation = allocation;
        return m_allocations[i].id;
      }
    }

    // otherwise push to end
    AllocationInfo info;
    info.id.index       = (uint32_t)m_allocations.size();
    info.id.incarnation = 0;
    info.allocation     = allocation;

    m_allocations.push_back(info);

    return info.id;
  }

  void BlockDeviceMemoryAllocator::freeID(AllocationID id)
  {
    assert(m_allocations[id.index].id.isEqual(id));
    // set to invalid
    m_allocations[id.index].id.invalidate();

    // shrink if last
    if((size_t)id.index == m_allocations.size() - 1)
    {
      m_allocations.pop_back();
    }
  }

  nvvk::BlockDeviceMemoryAllocator::MemoryBlockIterator BlockDeviceMemoryAllocator::getBlock(const Allocation& allocation)
  {
    for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
    {
      MemoryBlock& block = *it;

      if(block.mem == allocation.mem)
      {
        return it;
      }
    }

    return m_blocks.end();
  }

  void BlockDeviceMemoryAllocator::init(VkDevice                                device,
                                        const VkPhysicalDeviceMemoryProperties* memoryProperties,
                                        VkDeviceSize                            blockSize,
                                        const VkAllocationCallbacks*            allocator)
  {
    m_device               = device;
    m_allocation_callbacks = allocator;
    m_memoryProperties     = memoryProperties;
    m_blockSize            = blockSize;
  }

  void BlockDeviceMemoryAllocator::freeAll()
  {
    for (const auto& it : m_blocks)
    {
      if (it.mapped)
      {
        vkUnmapMemory(m_device, it.mem);
      }
      vkFreeMemory(m_device, it.mem, m_allocation_callbacks);
    }

    m_allocations.clear();
    m_blocks.clear();
  }


  void BlockDeviceMemoryAllocator::deinit()
  {
    freeAll();
  }

  void BlockDeviceMemoryAllocator::setMaxAllocationSize(VkDeviceSize size)
  {
    m_maxAllocationSize = size;
  }

  VkDeviceSize BlockDeviceMemoryAllocator::getMaxAllocationSize() const
  {
    return m_maxAllocationSize;
  }

  float BlockDeviceMemoryAllocator::getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const
  {
    allocatedSize = m_allocatedSize;
    usedSize      = m_usedSize;

    return float(double(usedSize) / double(allocatedSize));
  }

  VkDevice BlockDeviceMemoryAllocator::getDevice() const
  {
    return m_device;
  }

  const VkAllocationCallbacks* BlockDeviceMemoryAllocator::getAllocationCallbacks() const
  {
    return m_allocation_callbacks;
  }

  nvvk::Allocation BlockDeviceMemoryAllocator::getAllocation(AllocationID id) const
  {
    assert(m_allocations[id.index].id.isEqual(id));
    return m_allocations[id.index].allocation;
  }

  const VkPhysicalDeviceMemoryProperties* BlockDeviceMemoryAllocator::getMemoryProperties() const
  {
    return m_memoryProperties;
  }

  AllocationID BlockDeviceMemoryAllocator::alloc(const VkMemoryRequirements&          memReqs,
                                                 VkMemoryPropertyFlags                memProps,
                                                 const VkMemoryDedicatedAllocateInfo* dedicated)
  {
    VkMemoryAllocateInfo memInfo;

    // Fill out allocation info structure
    if(memReqs.size > m_maxAllocationSize || !PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(*m_memoryProperties, memReqs, memProps, memInfo))
    {
      return AllocationID();
    }

    if(!dedicated)
    {
      // First try to find an existing memory block that we can use
      for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
      {
        MemoryBlock& block = *it;

        // Ignore blocks with the wrong memory type
        if(block.memoryTypeIndex != memInfo.memoryTypeIndex)
        {
          continue;
        }

        VkDeviceSize offset = block.currentOffset;

        // Align offset to requirement
        offset = ((offset + memReqs.alignment - 1) / memReqs.alignment) * memReqs.alignment;

        // Look for a block which has enough free space available
        if(offset + memReqs.size <= block.allocationSize)
        {
          block.currentOffset = offset + memInfo.allocationSize;
          block.allocationCount++;

          Allocation allocation;
          allocation.mem    = block.mem;
          allocation.offset = offset;
          allocation.size   = memReqs.size;

          m_usedSize += allocation.size;

          return createID(allocation);
        }
      }
    }

    // Could not find enough space in an existing block, so allocate a new one
    m_blocks.emplace_back();
    MemoryBlock& block = m_blocks.back();

    block.allocationSize = memInfo.allocationSize = std::max(m_blockSize, memReqs.size);
    block.memoryTypeIndex                         = memInfo.memoryTypeIndex;

    if(dedicated)
    {
      block.allocationSize = memReqs.size;
      memInfo.pNext        = dedicated;
    }

    const VkResult res = vkAllocateMemory(m_device, &memInfo, m_allocation_callbacks, &block.mem);

    if(res == VK_SUCCESS)
    {
      m_allocatedSize += block.allocationSize;

      block.currentOffset   = memReqs.size;
      block.allocationCount = 1;
      block.mapCount        = 0;
      block.mapped          = nullptr;
      block.mappable        = (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

      Allocation allocation;
      allocation.mem  = block.mem;
      allocation.size = memReqs.size;

      m_usedSize += allocation.size;

      return createID(allocation);
    }
    else
    {
      m_blocks.pop_back();

      return AllocationID();
    }
  }

  void BlockDeviceMemoryAllocator::free(AllocationID allocationID)
  {
    Allocation allocation = getAllocation(allocationID);
    freeID(allocationID);

    m_usedSize -= allocation.size;

    auto it = getBlock(allocation);
    assert(it != m_blocks.end());

    MemoryBlock& block = *it;
    if(--block.allocationCount == 0)
    {
      assert(!block.mapped);
      vkFreeMemory(m_device, block.mem, m_allocation_callbacks);

      m_allocatedSize -= block.allocationSize;

      m_blocks.erase(it);
    }
  }


  void* BlockDeviceMemoryAllocator::map(AllocationID allocationID)
  {
    Allocation allocation = getAllocation(allocationID);
    auto       it         = getBlock(allocation);
    assert(it != m_blocks.end());

    MemoryBlock& block = *it;
    assert(block.mappable);
    block.mapCount++;
    if(!block.mapped)
    {
      VkResult result = vkMapMemory(m_device, block.mem, 0, block.allocationSize, 0, (void**)&block.mapped);
      assert(result == VK_SUCCESS);
    }
    return block.mapped + allocation.offset;
  }

  void BlockDeviceMemoryAllocator::unmap(AllocationID allocationID)
  {
    Allocation allocation = getAllocation(allocationID);
    auto       it         = getBlock(allocation);
    assert(it != m_blocks.end());

    MemoryBlock& block = *it;
    assert(block.mapped);
    if(--block.mapCount == 0)
    {
      block.mapped = nullptr;
      vkUnmapMemory(m_device, block.mem);
    }
  }

  VkResult BlockDeviceMemoryAllocator::create(const VkImageCreateInfo& inCreateInfo,
                                              VkImage&                 image,
                                              AllocationID&            allocationID,
                                              VkMemoryPropertyFlags    memProps,
                                              bool                     useDedicated)
  {
    VkImageCreateInfo createInfo = inCreateInfo;

    assert(inCreateInfo.extent.width && inCreateInfo.extent.height && inCreateInfo.extent.depth);

    VkDedicatedAllocationImageCreateInfoNV dedicatedImage = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV};
    dedicatedImage.pNext                                  = createInfo.pNext;
    dedicatedImage.dedicatedAllocation                    = VK_TRUE;
    if(useDedicated)
    {
      createInfo.pNext = &dedicatedImage;
    }

    VkResult result = vkCreateImage(m_device, &createInfo, m_allocation_callbacks, &image);
    if(result != VK_SUCCESS)
      return result;


    VkMemoryRequirements2          memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    VkMemoryDedicatedRequirements  dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
    VkImageMemoryRequirementsInfo2 imageReqs     = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
    if(useDedicated)
    {
      imageReqs.image = image;
      memReqs.pNext   = &dedicatedRegs;
      vkGetImageMemoryRequirements2(m_device, &imageReqs, &memReqs);
    }
    else
    {
      vkGetImageMemoryRequirements(m_device, image, &memReqs.memoryRequirements);
    }

    VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV};
    dedicatedInfo.image                         = image;

    allocationID          = alloc(memReqs.memoryRequirements, memProps, useDedicated ? &dedicatedInfo : nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if(allocation.mem == VK_NULL_HANDLE)
    {
      vkDestroyImage(m_device, image, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    result = vkBindImageMemory(m_device, image, allocation.mem, allocation.offset);
    if(result != VK_SUCCESS)
    {
      vkDestroyImage(m_device, image, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }
  VkResult BlockDeviceMemoryAllocator::create(const VkBufferCreateInfo& inCreateInfo,
                                              VkBuffer&                 buffer,
                                              AllocationID&             allocationID,
                                              VkMemoryPropertyFlags     memProps,
                                              bool                      useDedicated)
  {
    VkBufferCreateInfo createInfo = inCreateInfo;

    assert(createInfo.size);

    VkDedicatedAllocationBufferCreateInfoNV dedicatedBuffer = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV};
    dedicatedBuffer.pNext                                   = createInfo.pNext;
    dedicatedBuffer.dedicatedAllocation                     = VK_TRUE;
    if(useDedicated)
    {
      createInfo.pNext = &dedicatedBuffer;
    }

    VkResult result = vkCreateBuffer(m_device, &createInfo, m_allocation_callbacks, &buffer);
    if(result != VK_SUCCESS)
    {
      return result;
    }

    VkMemoryRequirements2           memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    VkMemoryDedicatedRequirements   dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
    VkBufferMemoryRequirementsInfo2 bufferReqs    = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
    if(useDedicated)
    {
      bufferReqs.buffer = buffer;
      memReqs.pNext     = &dedicatedRegs;
      vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);
    }
    else
    {
      vkGetBufferMemoryRequirements(m_device, buffer, &memReqs.memoryRequirements);
    }

    VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV};
    dedicatedInfo.buffer                        = buffer;

    allocationID          = alloc(memReqs.memoryRequirements, memProps, useDedicated ? &dedicatedInfo : nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if(allocation.mem == VK_NULL_HANDLE)
    {
      vkDestroyBuffer(m_device, buffer, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    result = vkBindBufferMemory(m_device, buffer, allocation.mem, allocation.offset);
    if(result != VK_SUCCESS)
    {
      vkDestroyBuffer(m_device, buffer, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }
  
#if VK_NV_ray_tracing
  VkResult BlockDeviceMemoryAllocator::create(const VkAccelerationStructureCreateInfoNV& createInfo,
                                              VkAccelerationStructureNV&                 accel,
                                              AllocationID&                              allocationID,
                                              VkMemoryPropertyFlags memProps /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/,
                                              bool                  useDedicated /*= false*/)
  {
    VkResult result = vkCreateAccelerationStructureNV(m_device, &createInfo, m_allocation_callbacks, &accel);
    if(result != VK_SUCCESS)
    {
      return result;
    }

    VkMemoryRequirements2                           memReqs = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    VkAccelerationStructureMemoryRequirementsInfoNV memInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
    memInfo.accelerationStructure = accel;
    vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memInfo, &memReqs);

    allocationID          = alloc(memReqs.memoryRequirements, memProps, nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if(allocation.mem == VK_NULL_HANDLE)
    {
      vkDestroyAccelerationStructureNV(m_device, accel, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    VkBindAccelerationStructureMemoryInfoNV bind = {VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV};
    bind.accelerationStructure                   = accel;
    bind.memory                                  = allocation.mem;
    bind.memoryOffset                            = allocation.offset;

    result = vkBindAccelerationStructureMemoryNV(m_device, 1, &bind);
    if(result != VK_SUCCESS)
    {
      vkDestroyAccelerationStructureNV(m_device, accel, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }
#endif

  //////////////////////////////////////////////////////////////////////////

  void MemoryBlockManager::init(const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize /*= 32 * 1024 * 1024*/, VkDeviceSize maxAllocationSize)
  {
    m_memoryProperties  = memoryProperties;
    m_maxAllocationSize = maxAllocationSize;
    m_blockSize         = blockSize;
    m_blocks.clear();
  }

  MemoryBlockManager::Entry MemoryBlockManager::add(const VkMemoryRequirements& memReqs,
                                                    VkMemoryPropertyFlags memProps /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
  {
    MemoryBlockManager::Entry entry;

    VkMemoryAllocateInfo memInfo;

    // Fill out allocation info structure
    if(memReqs.size > m_maxAllocationSize || !PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(*m_memoryProperties, memReqs, memProps, memInfo))
    {
      return entry;
    }

    // find open block

    size_t idx = 0;
    for(auto& block : m_blocks)
    {

      if(block.memAllocate.memoryTypeIndex != memInfo.memoryTypeIndex || block.memAllocate.allocationSize > m_blockSize)
      {
        idx++;
        continue;
      }

      VkDeviceSize offset = block.memAllocate.allocationSize;

      // Align offset to requirement
      offset = ((offset + memReqs.alignment - 1) / memReqs.alignment) * memReqs.alignment;

      // update the three allocation infos, whatever the user ends up using
      block.memAllocate.allocationSize = offset + memReqs.size;
      block.memReqs.size               = block.memAllocate.allocationSize;
      block.memProps |= memProps;

      entry.blockIdx = idx;
      entry.offset   = offset;

      return entry;
    }

    Block block;
    block.memAllocate = memInfo;
    block.memProps    = memProps;
    block.memReqs     = memReqs;

    entry.blockIdx = m_blocks.size();
    entry.offset   = 0;

    m_blocks.push_back(block);

    return entry;
  }

  void MemoryBlockManager::clear()
  {
    m_blocks.clear();
  }

  const size_t MemoryBlockManager::getBlockCount() const
  {
    return m_blocks.size();
  }

  const nvvk::MemoryBlockManager::Block& MemoryBlockManager::getBlock(size_t idx) const
  {
    return m_blocks[idx];
  }

  VkBindImageMemoryInfo MemoryBlockManager::makeBind(VkImage image, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const
  {
    const Allocation&     allocation = blockAllocations[entry.blockIdx];
    VkBindImageMemoryInfo info       = {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
    info.image                       = image;
    info.memory                      = allocation.mem;
    info.memoryOffset                = allocation.offset + entry.offset;
    return info;
  }

  VkBindBufferMemoryInfo MemoryBlockManager::makeBind(VkBuffer buffer, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const
  {
    const Allocation&      allocation = blockAllocations[entry.blockIdx];
    VkBindBufferMemoryInfo info       = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
    info.buffer                       = buffer;
    info.memory                       = allocation.mem;
    info.memoryOffset                 = allocation.offset + entry.offset;
    return info;
  }

#if VK_NV_ray_tracing
  VkBindAccelerationStructureMemoryInfoNV MemoryBlockManager::makeBind(VkAccelerationStructureNV        accel,
                                                                       const MemoryBlockManager::Entry& entry,
                                                                       const Allocation* blockAllocations) const
  {
    const Allocation&                       allocation = blockAllocations[entry.blockIdx];
    VkBindAccelerationStructureMemoryInfoNV info       = {VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV};
    info.accelerationStructure                         = accel;
    info.memory                                        = allocation.mem;
    info.memoryOffset                                  = allocation.offset + entry.offset;
    return info;
  }
#endif

}
