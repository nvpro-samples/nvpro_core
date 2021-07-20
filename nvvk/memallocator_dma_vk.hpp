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

#pragma once

#include "memallocator_vk.hpp"
#include "memorymanagement_vk.hpp"

namespace nvvk {

class DeviceMemoryAllocator;

/**
 \class nvvk::DMAMemoryAllocator
 nvvk::DMAMemoryAllocator is  using nvvk::DeviceMemoryAllocator internally.
 nvvk::DeviceMemoryAllocator derives from nvvk::MemAllocator as well, so this class here is for those prefering a reduced wrapper;
*/
class DMAMemoryAllocator : public MemAllocator
{
public:
  DMAMemoryAllocator() = default;
  explicit DMAMemoryAllocator(nvvk::DeviceMemoryAllocator* dma) { init(dma); }
  virtual ~DMAMemoryAllocator() { deinit(); }

  bool init(nvvk::DeviceMemoryAllocator* dma)
  {
    m_dma = dma;
    return m_dma != nullptr;
  }
  void deinit() { m_dma = nullptr; }

  // Implement MemAllocator interface
  virtual MemHandle allocMemory(const MemAllocateInfo& allocInfo, VkResult* pResult = nullptr) override { return m_dma->allocMemory(allocInfo, pResult); }
  virtual void      freeMemory(MemHandle memHandle) override { return m_dma->freeMemory(memHandle); }
  virtual MemInfo   getMemoryInfo(MemHandle memHandle) const override { return m_dma->getMemoryInfo(memHandle); }
  virtual void*     map(MemHandle memHandle, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE, VkResult* pResult = nullptr) override { return m_dma->map(memHandle, offset, size, pResult); }
  virtual void      unmap(MemHandle memHandle) override { return m_dma->unmap(memHandle); }

  virtual VkDevice         getDevice() const override { return m_dma->getDevice(); }
  virtual VkPhysicalDevice getPhysicalDevice() const override{ return m_dma->getPhysicalDevice(); }

  // Utility function
  AllocationID getAllocationID(MemHandle memHandle) const { return m_dma->getAllocationID(memHandle); }

private:
  nvvk::DeviceMemoryAllocator*     m_dma;
};

}  // namespace nvvk
