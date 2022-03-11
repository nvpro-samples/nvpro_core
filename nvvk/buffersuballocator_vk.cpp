/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
* SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
* SPDX-License-Identifier: Apache-2.0
*/


#include <assert.h>
#include "buffersuballocator_vk.hpp"
#include "debug_util_vk.hpp"
#include "error_vk.hpp"

namespace nvvk {

//////////////////////////////////////////////////////////////////////////

void BufferSubAllocator::init(MemAllocator*                memAllocator,
                              VkDeviceSize                 blockSize,
                              VkBufferUsageFlags           bufferUsageFlags,
                              VkMemoryPropertyFlags        memPropFlags,
                              bool                         mapped,
                              const std::vector<uint32_t>& sharingQueueFamilyIndices)
{
  assert(!m_device);
  m_memAllocator     = memAllocator;
  m_device           = memAllocator->getDevice();
  m_blockSize        = std::min(blockSize, ((uint64_t(1) << Handle::BLOCKBITS) - 1) * uint64_t(BASE_ALIGNMENT));
  m_bufferUsageFlags = bufferUsageFlags;
  m_memoryPropFlags  = memPropFlags;
  m_memoryTypeIndex  = ~0;
  m_keepLastBlock    = true;
  m_mapped           = mapped;
  m_sharingQueueFamilyIndices = sharingQueueFamilyIndices;

  m_freeBlockIndex = INVALID_ID_INDEX;
  m_usedSize       = 0;
  m_allocatedSize  = 0;
}

void BufferSubAllocator::deinit()
{
  if(!m_memAllocator)
    return;

  free(false);

  m_blocks.clear();
  m_memAllocator = nullptr;
}

BufferSubAllocator::Handle BufferSubAllocator::subAllocate(VkDeviceSize size, uint32_t align)
{
  uint32_t usedOffset;
  uint32_t usedSize;
  uint32_t usedAligned;

  uint32_t blockIndex = INVALID_ID_INDEX;

  // if size either doesn't fit in the bits within the handle
  // or we are bigger than the default block size, we use a full dedicated block
  // for this allocation
  bool isDedicated = Handle::needsDedicated(size, align) || size > m_blockSize;

  if(!isDedicated)
  {
    // Find the first non-dedicated block that can fit the allocation
    for(uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++)
    {
      Block& block = m_blocks[i];
      if(!block.isDedicated && block.buffer && block.range.subAllocate((uint32_t)size, align, usedOffset, usedAligned, usedSize))
      {
        blockIndex = block.index;
        break;
      }
    }
  }

  if(blockIndex == INVALID_ID_INDEX)
  {
    if(m_freeBlockIndex != INVALID_ID_INDEX)
    {
      Block& block     = m_blocks[m_freeBlockIndex];
      m_freeBlockIndex = setIndexValue(block.index, m_freeBlockIndex);

      blockIndex = block.index;
    }
    else
    {
      uint32_t newIndex = (uint32_t)m_blocks.size();
      m_blocks.resize(m_blocks.size() + 1);
      Block& block = m_blocks[newIndex];
      block.index  = newIndex;

      blockIndex = newIndex;
    }

    Block& block = m_blocks[blockIndex];
    block.size   = std::max(m_blockSize, size);
    if(!isDedicated)
    {
      // only adjust size if not dedicated.
      // warning this lowers from 64 bit to 32 bit size, which should be fine given
      // such big allocations will trigger the dedicated path
      block.size = block.range.alignedSize((uint32_t)block.size);
    }

    VkResult result = allocBlock(block, blockIndex, block.size);
    NVVK_CHECK(result);
    if(result != VK_SUCCESS)
    {
      freeBlock(block);
      return Handle();
    }

    block.isDedicated = isDedicated;

    if(!isDedicated)
    {
      // Dedicated blocks don't allow for subranges, so don't initialize the range allocator
      block.range.init((uint32_t)block.size);
      block.range.subAllocate((uint32_t)size, align, usedOffset, usedAligned, usedSize);
      m_regularBlocks++;
    }
  }

  Handle sub;
  if(!sub.setup(blockIndex, isDedicated ? 0 : usedOffset, isDedicated ? size : uint64_t(usedSize), isDedicated))
  {
    return Handle();
  }

  // append used space for stats
  m_usedSize += sub.getSize();

  return sub;
}

void BufferSubAllocator::subFree(Handle sub)
{
  if(!sub)
    return;

  Block& block       = getBlock(sub.blockIndex);
  bool   isDedicated = sub.isDedicated();
  if(!isDedicated)
  {
    block.range.subFree(uint32_t(sub.getOffset()), uint32_t(sub.getSize()));
  }

  m_usedSize -= sub.getSize();

  if(isDedicated || (block.range.isEmpty() && (!m_keepLastBlock || m_regularBlocks > 1)))
  {
    if(!isDedicated)
    {
      m_regularBlocks--;
    }
    freeBlock(block);
  }
}

float BufferSubAllocator::getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const
{
  allocatedSize = m_allocatedSize;
  usedSize      = m_usedSize;

  return float(double(usedSize) / double(allocatedSize));
}

bool BufferSubAllocator::fitsInAllocated(VkDeviceSize size, uint32_t alignment) const
{
  if(Handle::needsDedicated(size, alignment))
  {
    return false;
  }

  for(const auto& block : m_blocks)
  {
    if(block.buffer && !block.isDedicated)
    {
      if(block.range.isAvailable((uint32_t)size, (uint32_t)alignment))
      {
        return true;
      }
    }
  }

  return false;
}

void BufferSubAllocator::free(bool onlyEmpty)
{
  for(uint32_t i = 0; i < (uint32_t)m_blocks.size(); i++)
  {
    Block& block = m_blocks[i];
    if(block.buffer && (!onlyEmpty || (!block.isDedicated && block.range.isEmpty())))
    {
      freeBlock(block);
    }
  }

  if(!onlyEmpty)
  {
    m_blocks.clear();
    m_freeBlockIndex = INVALID_ID_INDEX;
  }
}

void BufferSubAllocator::freeBlock(Block& block)
{
  m_allocatedSize -= block.size;

  vkDestroyBuffer(m_device, block.buffer, nullptr);
  if(block.mapping)
  {
    m_memAllocator->unmap(block.memory);
  }
  m_memAllocator->freeMemory(block.memory);

  if(!block.isDedicated)
  {
    block.range.deinit();
  }
  block.memory      = NullMemHandle;
  block.buffer      = VK_NULL_HANDLE;
  block.mapping     = nullptr;
  block.isDedicated = false;

  // update the block.index with the current head of the free list
  // pop its old value
  m_freeBlockIndex = setIndexValue(block.index, m_freeBlockIndex);
}

VkResult BufferSubAllocator::allocBlock(Block& block, uint32_t index, VkDeviceSize size)
{

  std::string debugName = m_debugName + ":block:" + std::to_string(index);

  VkResult           result;
  VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfo.size               = size;
  createInfo.usage              = m_bufferUsageFlags;
  createInfo.sharingMode = m_sharingQueueFamilyIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
  createInfo.pQueueFamilyIndices   = m_sharingQueueFamilyIndices.data();
  createInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_sharingQueueFamilyIndices.size());

  VkBuffer buffer = VK_NULL_HANDLE;
  result          = vkCreateBuffer(m_device, &createInfo, nullptr, &buffer);
  if(result != VK_SUCCESS)
  {
    NVVK_CHECK(result);
    return result;
  }
  nvvk::DebugUtil(m_device).setObjectName(buffer, debugName);

  VkMemoryRequirements2           memReqs    = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkBufferMemoryRequirementsInfo2 bufferReqs = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};

  bufferReqs.buffer = buffer;
  vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);


  if(m_memoryTypeIndex == ~0)
  {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_memAllocator->getPhysicalDevice(), &memoryProperties);

    VkMemoryPropertyFlags memProps = m_memoryPropFlags;

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
    assert(0 && "could not find memoryTypeIndex\n");
    vkDestroyBuffer(m_device, buffer, nullptr);
    return VK_ERROR_INCOMPATIBLE_DRIVER;
  }

  MemAllocateInfo memAllocateInfo(memReqs.memoryRequirements, m_memoryPropFlags, false);
  memAllocateInfo.setDebugName(debugName);

  MemHandle memory = m_memAllocator->allocMemory(memAllocateInfo, &result);
  if(result != VK_SUCCESS)
  {
    assert(0 && "could not allocate buffer\n");
    vkDestroyBuffer(m_device, buffer, nullptr);
    return result;
  }

  MemAllocator::MemInfo memInfo = m_memAllocator->getMemoryInfo(memory);

  VkBindBufferMemoryInfo bindInfos = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
  bindInfos.buffer                 = buffer;
  bindInfos.memory                 = memInfo.memory;
  bindInfos.memoryOffset           = memInfo.offset;

  result = vkBindBufferMemory2(m_device, 1, &bindInfos);

  if(result == VK_SUCCESS)
  {
    if(m_mapped)
    {
      block.mapping = m_memAllocator->mapT<uint8_t>(memory);
    }
    else
    {
      block.mapping = nullptr;
    }

    if(!m_mapped || block.mapping)
    {
      if(m_bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
      {
        VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer                    = buffer;
        block.address                  = vkGetBufferDeviceAddress(m_device, &info);
      }

      block.memory = memory;
      block.buffer = buffer;
      m_allocatedSize += block.size;
      return result;
    }
  }

  // error case
  NVVK_CHECK(result);
  vkDestroyBuffer(m_device, buffer, nullptr);
  m_memAllocator->freeMemory(memory);
  return result;
}


}  // namespace nvvk
