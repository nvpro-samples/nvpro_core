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

#include <nvvk/stagingmemorymanager_vk.hpp>

#include <nvh/nvprint.hpp>
#include <nvvk/debug_util_vk.hpp>
#include <nvvk/error_vk.hpp>

namespace nvvk {

void StagingMemoryManager::init(MemAllocator* memAllocator, VkDeviceSize stagingBlockSize /*= 64 * 1024 * 1024*/)
{
  assert(!m_device);
  m_device = memAllocator->getDevice();

  m_subToDevice.init(memAllocator, stagingBlockSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
  m_subFromDevice.init(memAllocator, stagingBlockSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                       true);

  m_freeStagingIndex = INVALID_ID_INDEX;
  m_stagingIndex     = newStagingIndex();

  setFreeUnusedOnRelease(true);
}

void StagingMemoryManager::deinit()
{
  if(!m_device)
    return;

  free(false);

  m_subFromDevice.deinit();
  m_subToDevice.deinit();

  m_sets.clear();
  m_device = VK_NULL_HANDLE;
}

bool StagingMemoryManager::fitsInAllocated(VkDeviceSize size, bool toDevice /*= true*/) const
{
  return toDevice ? m_subToDevice.fitsInAllocated(size) : m_subFromDevice.fitsInAllocated(size);
}

void* StagingMemoryManager::cmdToImage(VkCommandBuffer                 cmd,
                                    VkImage                         image,
                                    const VkOffset3D&               offset,
                                    const VkExtent3D&               extent,
                                    const VkImageSubresourceLayers& subresource,
                                    VkDeviceSize                    size,
                                    const void*                     data,
                                    VkImageLayout                   layout)
{
  if(!image)
    return nullptr;

  VkBuffer     srcBuffer;
  VkDeviceSize srcOffset;

  void* mapping = getStagingSpace(size, srcBuffer, srcOffset, true);

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

  vkCmdCopyBufferToImage(cmd, srcBuffer, image, layout, 1, &cpy);

  return data ? nullptr : mapping;
}

void* StagingMemoryManager::cmdToBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
  if(!size || !buffer)
  {
    return nullptr;
  }

  VkBuffer     srcBuffer;
  VkDeviceSize srcOffset;

  void* mapping = getStagingSpace(size, srcBuffer, srcOffset, true);

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

const void* StagingMemoryManager::cmdFromBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
{
  VkBuffer     dstBuffer;
  VkDeviceSize dstOffset;
  void*        mapping = getStagingSpace(size, dstBuffer, dstOffset, false);

  VkBufferCopy cpy;
  cpy.size      = size;
  cpy.srcOffset = offset;
  cpy.dstOffset = dstOffset;

  vkCmdCopyBuffer(cmd, buffer, dstBuffer, 1, &cpy);

  return mapping;
}

const void* StagingMemoryManager::cmdFromImage(VkCommandBuffer                 cmd,
                                            VkImage                         image,
                                            const VkOffset3D&               offset,
                                            const VkExtent3D&               extent,
                                            const VkImageSubresourceLayers& subresource,
                                            VkDeviceSize                    size,
                                            VkImageLayout                   layout)
{
  VkBuffer     dstBuffer;
  VkDeviceSize dstOffset;
  void*        mapping = getStagingSpace(size, dstBuffer, dstOffset, false);

  VkBufferImageCopy cpy;
  cpy.bufferOffset      = dstOffset;
  cpy.bufferRowLength   = 0;
  cpy.bufferImageHeight = 0;
  cpy.imageSubresource  = subresource;
  cpy.imageOffset       = offset;
  cpy.imageExtent       = extent;

  vkCmdCopyImageToBuffer(cmd, image, layout, dstBuffer, 1, &cpy);

  return mapping;
}

void StagingMemoryManager::finalizeResources(VkFence fence)
{
  if(m_sets[m_stagingIndex].entries.empty())
    return;

  m_sets[m_stagingIndex].fence     = fence;
  m_sets[m_stagingIndex].manualSet = false;
  m_stagingIndex                   = newStagingIndex();
}

StagingMemoryManager::SetID StagingMemoryManager::finalizeResourceSet()
{
  SetID setID;

  if(m_sets[m_stagingIndex].entries.empty())
    return setID;

  setID.index = m_stagingIndex;

  m_sets[m_stagingIndex].fence     = nullptr;
  m_sets[m_stagingIndex].manualSet = true;
  m_stagingIndex                   = newStagingIndex();

  return setID;
}

void* StagingMemoryManager::getStagingSpace(VkDeviceSize size, VkBuffer& buffer, VkDeviceSize& offset, bool toDevice)
{
  assert(m_sets[m_stagingIndex].index == m_stagingIndex && "illegal index, did you forget finalizeResources");

  BufferSubAllocator::Handle handle = toDevice ? m_subToDevice.subAllocate(size) : m_subFromDevice.subAllocate(size);
  assert(handle);

  BufferSubAllocator::Binding info = toDevice ? m_subToDevice.getSubBinding(handle) : m_subFromDevice.getSubBinding(handle);
  buffer = info.buffer;
  offset = info.offset;

  // append used space to current staging set list
  m_sets[m_stagingIndex].entries.push_back({handle, toDevice});

  return toDevice ? m_subToDevice.getSubMapping(handle) : m_subFromDevice.getSubMapping(handle);
}

void StagingMemoryManager::releaseResources(uint32_t stagingID)
{
  if(stagingID == INVALID_ID_INDEX)
    return;

  StagingSet& set = m_sets[stagingID];
  assert(set.index == stagingID);

  // free used allocation ranges
  for(auto& itentry : set.entries)
  {
    if(itentry.toDevice)
    {
      m_subToDevice.subFree(itentry.handle);
    }
    else
    {
      m_subFromDevice.subFree(itentry.handle);
    }
  }
  set.entries.clear();

  // update the set.index with the current head of the free list
  // pop its old value
  m_freeStagingIndex = setIndexValue(set.index, m_freeStagingIndex);
}

void StagingMemoryManager::releaseResources()
{
  for(auto& itset : m_sets)
  {
    if(!itset.entries.empty() && !itset.manualSet && (!itset.fence || vkGetFenceStatus(m_device, itset.fence) == VK_SUCCESS))
    {
      releaseResources(itset.index);
      itset.fence     = NULL;
      itset.manualSet = false;
    }
  }
  // special case for ease of use if there is only one
  if(m_stagingIndex == 0 && m_freeStagingIndex == 0)
  {
    m_freeStagingIndex = setIndexValue(m_sets[0].index, 0);
  }
}


float StagingMemoryManager::getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const
{
  VkDeviceSize aSize = 0;
  VkDeviceSize uSize = 0;
  m_subFromDevice.getUtilization(aSize, uSize);

  allocatedSize = aSize;
  usedSize      = uSize;
  m_subToDevice.getUtilization(aSize, uSize);
  allocatedSize += aSize;
  usedSize += uSize;

  return float(double(usedSize) / double(allocatedSize));
}

void StagingMemoryManager::free(bool unusedOnly)
{
  m_subToDevice.free(unusedOnly);
  m_subFromDevice.free(unusedOnly);
}

uint32_t StagingMemoryManager::newStagingIndex()
{
  // find free slot
  if(m_freeStagingIndex != INVALID_ID_INDEX)
  {
    uint32_t newIndex = m_freeStagingIndex;
    // this updates the free link-list
    m_freeStagingIndex = setIndexValue(m_sets[newIndex].index, newIndex);
    assert(m_sets[newIndex].index == newIndex);
    return m_sets[newIndex].index;
  }

  // otherwise push to end
  uint32_t newIndex = (uint32_t)m_sets.size();

  StagingSet info;
  info.index = newIndex;
  m_sets.push_back(info);

  assert(m_sets[newIndex].index == newIndex);
  return newIndex;
}

}  // namespace nvvk