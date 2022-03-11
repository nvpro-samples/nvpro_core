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

#include "memallocator_dedicated_vk.hpp"

#include "error_vk.hpp"
#include "debug_util_vk.hpp"

#include <cassert>

namespace nvvk {

class DedicatedMemoryHandle : public MemHandleBase
{
public:
  DedicatedMemoryHandle()                             = default;
  DedicatedMemoryHandle(const DedicatedMemoryHandle&) = default;
  DedicatedMemoryHandle(DedicatedMemoryHandle&&)      = default;

  DedicatedMemoryHandle& operator=(const DedicatedMemoryHandle&) = default;
  DedicatedMemoryHandle& operator=(DedicatedMemoryHandle&&) = default;

  VkDeviceMemory getMemory() const { return m_memory; }
  VkDeviceSize   getSize() const { return m_size; }

private:
  friend class DedicatedMemoryAllocator;
  DedicatedMemoryHandle(VkDeviceMemory memory, VkDeviceSize size)
      : m_memory(memory)
      , m_size(size)
  {
  }

  VkDeviceMemory m_memory;
  VkDeviceSize   m_size;
};

DedicatedMemoryHandle* castDedicatedMemoryHandle(MemHandle memHandle)
{
  if(!memHandle)
    return nullptr;
#ifndef NDEBUG
  auto dedicatedMemHandle = static_cast<DedicatedMemoryHandle*>(memHandle);
#else
  auto dedicatedMemHandle = dynamic_cast<DedicatedMemoryHandle*>(memHandle);
  assert(dedicatedMemHandle);
#endif

  return dedicatedMemHandle;
}

DedicatedMemoryAllocator::DedicatedMemoryAllocator(VkDevice device, VkPhysicalDevice physDevice)
{
  init(device, physDevice);
}

DedicatedMemoryAllocator::~DedicatedMemoryAllocator()
{
  deinit();
}

bool DedicatedMemoryAllocator::init(VkDevice device, VkPhysicalDevice physDevice)
{
  m_device         = device;
  m_physicalDevice = physDevice;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalMemoryProperties);

  return true;
}

void DedicatedMemoryAllocator::deinit()
{
  m_device = NULL;
}

MemHandle DedicatedMemoryAllocator::allocMemory(const MemAllocateInfo& allocInfo, VkResult *pResult)
{
  MemAllocateInfo   localInfo(allocInfo);
  localInfo.setAllocationFlags(allocInfo.getAllocationFlags() | m_flags);

  BakedAllocateInfo bakedInfo;
  fillBakedAllocateInfo(m_physicalMemoryProperties, localInfo, bakedInfo);

  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkResult result = vkAllocateMemory(m_device, &bakedInfo.memAllocInfo, nullptr, &memory);
  NVVK_CHECK(result);
  if (pResult)
  {
    *pResult = result;
  }

  if(result == VK_SUCCESS)
  {
    auto dedicatedMemHandle = new DedicatedMemoryHandle(memory, bakedInfo.memAllocInfo.allocationSize);

    if(!allocInfo.getDebugName().empty())
    {
      const MemInfo& memInfo = getMemoryInfo(dedicatedMemHandle);
      nvvk::DebugUtil(m_device).setObjectName(memInfo.memory, localInfo.getDebugName());
    }

    return dedicatedMemHandle;
  }
  else
  {
    return NullMemHandle;
  }
}

void DedicatedMemoryAllocator::freeMemory(MemHandle memHandle)
{
  if(!memHandle)
    return;

  auto dedicatedHandle = castDedicatedMemoryHandle(memHandle);

  vkFreeMemory(m_device, dedicatedHandle->getMemory(), nullptr);

  delete dedicatedHandle;

  return;
}

MemAllocator::MemInfo DedicatedMemoryAllocator::getMemoryInfo(MemHandle memHandle) const
{
  auto dedicatedHandle = castDedicatedMemoryHandle(memHandle);

  return MemInfo{dedicatedHandle->getMemory(), 0, dedicatedHandle->getSize()};
}

void* DedicatedMemoryAllocator::map(MemHandle memHandle, VkDeviceSize offset, VkDeviceSize size, VkResult *pResult)
{
  auto  dedicatedHandle = castDedicatedMemoryHandle(memHandle);
  void* ptr             = nullptr;
  VkResult result = vkMapMemory(m_device, dedicatedHandle->getMemory(), offset, size, 0 /*VkMemoryFlags*/, &ptr);

  NVVK_CHECK(result);
  if(pResult)
  {
    *pResult = result;
  }

  return ptr;
}

void DedicatedMemoryAllocator::unmap(MemHandle memHandle)
{
  auto dedicatedHandle = castDedicatedMemoryHandle(memHandle);
  vkUnmapMemory(m_device, dedicatedHandle->getMemory());
}


VkDevice DedicatedMemoryAllocator::getDevice() const
{
  return m_device;
}


VkPhysicalDevice DedicatedMemoryAllocator::getPhysicalDevice() const
{
  return m_physicalDevice;
}


void DedicatedMemoryAllocator::setAllocateFlags(VkMemoryAllocateFlags flags)
{
  m_flags = flags;
}

}  // namespace nvvk
