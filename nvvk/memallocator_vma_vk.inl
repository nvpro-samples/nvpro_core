/*
 * Copyright (c) 2019-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2023, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vk_mem_alloc.h"
#include "error_vk.hpp"

#if defined(LINUX)
#include <signal.h>  // LINUX SIGTRAP
#endif

namespace nvvk {

class VMAMemoryHandle : public MemHandleBase
{
public:
  VMAMemoryHandle()                       = default;
  VMAMemoryHandle(const VMAMemoryHandle&) = default;
  VMAMemoryHandle(VMAMemoryHandle&&)      = default;

  VmaAllocation getAllocation() const { return m_allocation; }

private:
  friend class VMAMemoryAllocator;
  VMAMemoryHandle(VmaAllocation allocation)
      : m_allocation(allocation)
  {
  }

  VmaAllocation m_allocation;
};

inline VMAMemoryHandle* castVMAMemoryHandle(MemHandle memHandle)
{
  if(!memHandle)
    return nullptr;
#ifndef NDEBUG
  auto vmaMemHandle = static_cast<VMAMemoryHandle*>(memHandle);
#else
  auto vmaMemHandle = dynamic_cast<VMAMemoryHandle*>(memHandle);
  assert(vmaMemHandle);
#endif

  return vmaMemHandle;
}

inline VMAMemoryAllocator::VMAMemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vma)
{
  init(device, physicalDevice, vma);
}


inline VMAMemoryAllocator::~VMAMemoryAllocator()
{
  deinit();
}

inline bool VMAMemoryAllocator::init(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vma)
{
  m_device         = device;
  m_physicalDevice = physicalDevice;
  m_vma            = vma;
  return true;
}

inline void VMAMemoryAllocator::deinit()
{
  m_vma = 0;
}

inline MemHandle VMAMemoryAllocator::allocMemory(const MemAllocateInfo& allocInfo, VkResult* pResult)
{
  VmaAllocationCreateInfo vmaAllocInfo = {};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;  // the other members of VmaAllocationCreateInfo specify requirements.

  vmaAllocInfo.requiredFlags  = allocInfo.getMemoryProperties();
  vmaAllocInfo.memoryTypeBits = allocInfo.getMemoryRequirements().memoryTypeBits;

  vmaAllocInfo.priority = allocInfo.getPriority();

  // Not supported by VMA
  assert(!allocInfo.getExportable());
  assert(!allocInfo.getDeviceMask());

  VmaAllocation allocation = nullptr;
  VkResult      result     = VK_ERROR_UNKNOWN;

  if(allocInfo.getDedicatedBuffer())
  {
    vmaAllocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    result = vmaAllocateMemoryForBuffer(m_vma, allocInfo.getDedicatedBuffer(), &vmaAllocInfo, &allocation, nullptr);
  }
  else if(allocInfo.getDedicatedImage())
  {
    vmaAllocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    result = vmaAllocateMemoryForImage(m_vma, allocInfo.getDedicatedImage(), &vmaAllocInfo, &allocation, nullptr);
  }
  else
  {
    result = vmaAllocateMemory(m_vma, &allocInfo.getMemoryRequirements(), &vmaAllocInfo, &allocation, nullptr);
  }

#ifndef NDEBUG
  // !! VMA leaks finder!!
  // Call findLeak with the value showing in the leak report.
  // Add : #define VMA_DEBUG_LOG_FORMAT(format, ...) do { printf(format, __VA_ARGS__); printf("\n"); } while(false)
  //  - in the app where VMA_IMPLEMENTATION is defined, to have a leak report
  // See for m_AllocationList.m_pName "nv_alloc: ID" and use ID number in findLeak()
  static uint64_t counter{0};
  if(counter == m_leakID)
  {
    bool stop_here = true;
#if defined(_MSVC_LANG)
    __debugbreak();
#elif defined(LINUX)
    raise(SIGTRAP);
#endif
  }
  if(result == VK_SUCCESS)
  {
    std::string allocID = std::string("nv_alloc: ") + std::to_string(counter++);
    vmaSetAllocationName(m_vma, allocation, allocID.c_str());
  }
#endif  // !NDEBUG

  NVVK_CHECK(result);
  if(pResult)
  {
    *pResult = result;
  }
  if(result == VK_SUCCESS)
  {
    return new VMAMemoryHandle(allocation);
  }
  else
  {
    return NullMemHandle;
  }
}

inline void VMAMemoryAllocator::freeMemory(MemHandle memHandle)
{
  if(!memHandle)
    return;

  auto vmaHandle = castVMAMemoryHandle(memHandle);
  vmaFreeMemory(m_vma, vmaHandle->getAllocation());
}

inline MemAllocator::MemInfo VMAMemoryAllocator::getMemoryInfo(MemHandle memHandle) const
{
  auto vmaHandle = castVMAMemoryHandle(memHandle);

  VmaAllocationInfo allocInfo;
  vmaGetAllocationInfo(m_vma, vmaHandle->getAllocation(), &allocInfo);

  MemInfo memInfo;
  memInfo.memory = allocInfo.deviceMemory;
  memInfo.offset = allocInfo.offset;
  memInfo.size   = allocInfo.size;

  return memInfo;
}

inline void* VMAMemoryAllocator::map(MemHandle memHandle, VkDeviceSize offset, VkDeviceSize size, VkResult* pResult)
{
  auto vmaHandle = castVMAMemoryHandle(memHandle);

  void*    ptr;
  VkResult result = vmaMapMemory(m_vma, vmaHandle->getAllocation(), &ptr);
  NVVK_CHECK(result);
  if(pResult)
  {
    *pResult = result;
  }

  return ptr;
}

inline void VMAMemoryAllocator::unmap(MemHandle memHandle)
{
  auto vmaHandle = castVMAMemoryHandle(memHandle);

  vmaUnmapMemory(m_vma, vmaHandle->getAllocation());
}


inline VkDevice VMAMemoryAllocator::getDevice() const
{
  return m_device;
}

inline VkPhysicalDevice VMAMemoryAllocator::getPhysicalDevice() const
{
  return m_physicalDevice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline ResourceAllocatorVma::ResourceAllocatorVma(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize)
{
  init(instance, device, physicalDevice, stagingBlockSize);
}

inline ResourceAllocatorVma::~ResourceAllocatorVma()
{
  deinit();
}

inline void ResourceAllocatorVma::init(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize)
{
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice         = physicalDevice;
  allocatorInfo.device                 = device;
  allocatorInfo.instance               = instance;
  allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocatorInfo, &m_vma);

  m_memAlloc.reset(new VMAMemoryAllocator(device, physicalDevice, m_vma));
  ResourceAllocator::init(device, physicalDevice, m_memAlloc.get(), stagingBlockSize);
}

inline void ResourceAllocatorVma::deinit()
{
  ResourceAllocator::deinit();

  m_memAlloc.reset();
  vmaDestroyAllocator(m_vma);
  m_vma = nullptr;
}

}  // namespace nvvk
