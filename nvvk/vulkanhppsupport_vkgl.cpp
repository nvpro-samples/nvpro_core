/*
 * SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if NVP_SUPPORTS_OPENGL

#include "vulkanhppsupport_vkgl.hpp"

namespace nvvkpp {

ResourceAllocatorGLInterop::ResourceAllocatorGLInterop(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize)
{
  init(device, physicalDevice, stagingBlockSize);
}

ResourceAllocatorGLInterop::~ResourceAllocatorGLInterop()
{
  deinit();
}

void ResourceAllocatorGLInterop::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize)
{
  m_dmaGL = std::make_unique<nvvk::DeviceMemoryAllocatorGL>(device, physicalDevice);
  nvvkpp::ExportResourceAllocator::init(device, physicalDevice, m_dmaGL.get(), stagingBlockSize);

  // The staging will only use DMA, without export functionality.
  m_dma = std::make_unique<nvvk::DeviceMemoryAllocator>(device, physicalDevice);
  m_staging = std::make_unique<nvvk::StagingMemoryManager>(dynamic_cast<nvvk::MemAllocator*>(m_dma.get()), stagingBlockSize);
}

void ResourceAllocatorGLInterop::deinit()
{
  nvvkpp::ExportResourceAllocator::deinit();
  m_dmaGL.reset();
  m_dma.reset();
}

nvvk::AllocationGL ResourceAllocatorGLInterop::getAllocationGL(nvvk::MemHandle memHandle) const
{
  return m_dmaGL->getAllocationGL(m_dmaGL->getAllocationID(memHandle));
}

} // namespace nvvkpp

#endif
