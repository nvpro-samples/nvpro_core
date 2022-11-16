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

#include "vk_mem_alloc.h"

#ifndef MEMALLOCATOR_VMA_H_INCLUDED
#define MEMALLOCATOR_VMA_H_INCLUDED

#include "memallocator_vk.hpp"
#include "resourceallocator_vk.hpp"

namespace nvvk {
/**
 \class nvvk::VMAMemoryAllocator
 nvvk::VMAMemoryAllocator using the GPUOpen [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) underneath.
 As VMA comes as a header-only library, when using it you'll have to:
  1) provide _add_package_VMA() in your CMakeLists.txt
  2) put these lines into one of your compilation units:
  \code{.cpp}
       #define VMA_IMPLEMENTATION
       #include "vk_mem_alloc.h"
  \endcode
*/
class VMAMemoryAllocator : public MemAllocator
{
public:
  VMAMemoryAllocator() = default;
  inline explicit VMAMemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vma);
  inline virtual ~VMAMemoryAllocator();

  inline bool init(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vma);
  inline void deinit();

  inline MemHandle allocMemory(const MemAllocateInfo& allocInfo, VkResult* pResult = nullptr) override;
  inline void      freeMemory(MemHandle memHandle) override;
  inline MemInfo   getMemoryInfo(MemHandle memHandle) const override;
  inline void*     map(MemHandle memHandle, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE, VkResult* pResult = nullptr) override;
  inline void      unmap(MemHandle memHandle) override;

  inline VkDevice         getDevice() const override;
  inline VkPhysicalDevice getPhysicalDevice() const override;

  inline void findLeak(uint64_t leakID) { m_leakID = leakID; }

private:
  VmaAllocator     m_vma{0};
  VkDevice         m_device{nullptr};
  VkPhysicalDevice m_physicalDevice{nullptr};
  uint64_t         m_leakID{~0U};
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 \class nvvk::ResourceAllocatorVMA
 nvvk::ResourceAllocatorVMA is a convencience class creating, initializing and owning a nvvk::VmaAllocator
 and associated nvvk::MemAllocator object. 
*/
class ResourceAllocatorVma : public ResourceAllocator
{
public:
  ResourceAllocatorVma() = default;
  ResourceAllocatorVma(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  virtual ~ResourceAllocatorVma();

  void init(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  void deinit();

protected:
  VmaAllocator                  m_vma{nullptr};
  std::unique_ptr<MemAllocator> m_memAlloc;
};

}  // namespace nvvk

#include "memallocator_vma_vk.inl"

#endif
