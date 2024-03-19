/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <memory>
#include "vk_mem_alloc.h"
#include "nvvk/resourceallocator_vk.hpp"
#include "nvvk/context_vk.hpp"
#include "nvvk/memallocator_vma_vk.hpp"

namespace nvvkhl {
/** @DOC_START
# class nvvkhl::AllocVma

>  This class is an element of the application that is responsible for the resource allocation. It is using the `VMA` library to allocate buffers, images and acceleration structures.

This allocator uses VMA (Vulkan Memory Allocator) to allocate buffers, images and acceleration structures. It is using the `nvvk::ResourceAllocator` to manage the allocation and deallocation of the resources.
  
@DOC_END */

class AllocVma : public nvvk::ResourceAllocator
{
public:
  explicit AllocVma(const nvvk::Context* context)
  {
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice         = context->m_physicalDevice;
    allocator_info.device                 = context->m_device;
    allocator_info.instance               = context->m_instance;
    allocator_info.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    init(allocator_info);
  }
  explicit AllocVma(const VmaAllocatorCreateInfo& allocatorInfo) { init(allocatorInfo); }
  ~AllocVma() override { deinit(); }

  // Use the following to trace
  // #define VMA_DEBUG_LOG(format, ...) do { printf(format, __VA_ARGS__); printf("\n"); } while(false)
  void findLeak(uint64_t leak) { m_vma->findLeak(leak); }

  VmaAllocator vma() { return m_vmaAlloc; }

private:
  void init(const VmaAllocatorCreateInfo& allocatorInfo)
  {
    vmaCreateAllocator(&allocatorInfo, &m_vmaAlloc);
    m_vma = std::make_unique<nvvk::VMAMemoryAllocator>(allocatorInfo.device, allocatorInfo.physicalDevice, m_vmaAlloc);
    nvvk::ResourceAllocator::init(allocatorInfo.device, allocatorInfo.physicalDevice, m_vma.get());
  }

  void deinit()
  {
    releaseStaging();
    vmaDestroyAllocator(m_vmaAlloc);
    m_vmaAlloc = nullptr;
    m_vma->deinit();
    nvvk::ResourceAllocator::deinit();
  }

  std::unique_ptr<nvvk::VMAMemoryAllocator> m_vma;  // The memory allocator
  VmaAllocator                              m_vmaAlloc{};
};

}  // namespace nvvkhl
