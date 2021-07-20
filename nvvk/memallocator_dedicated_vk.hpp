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

namespace nvvk {

/**
 \class nvvk::DedicatedMemoryAllocator
 nvvk::DedicatedMemoryAllocator is a simple implementation of the MemAllocator interface, using
 one vkDeviceMemory allocation per allocMemory() call. The simplicity of the implementation is
 bought with potential slowness (vkAllocateMemory tends to be very slow) and running
 out of operating system resources quickly (as some OSs limit the number of physical
 memory allocations per process).
*/
class DedicatedMemoryAllocator : public MemAllocator
{
public:
  DedicatedMemoryAllocator() = default;
  explicit DedicatedMemoryAllocator(VkDevice device, VkPhysicalDevice physDevice);
  virtual ~DedicatedMemoryAllocator();

  bool init(VkDevice device, VkPhysicalDevice physDevice);
  void deinit();

  virtual MemHandle allocMemory(const MemAllocateInfo& allocInfo, VkResult* pResult = nullptr) override;
  virtual void      freeMemory(MemHandle memHandle) override;
  virtual MemInfo   getMemoryInfo(MemHandle memHandle) const override;
  virtual void*     map(MemHandle memHandle, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE, VkResult* pResult = nullptr) override;
  virtual void      unmap(MemHandle memHandle) override;

  virtual VkDevice         getDevice() const override;
  virtual VkPhysicalDevice getPhysicalDevice() const override;

  void setAllocateFlags(VkMemoryAllocateFlags flags);

private:
  VkDevice                         m_device{NULL};
  VkPhysicalDevice                 m_physicalDevice{NULL};
  VkPhysicalDeviceMemoryProperties m_physicalMemoryProperties;
  VkMemoryAllocateFlags            m_flags{0};
};

}  // namespace nvvk
