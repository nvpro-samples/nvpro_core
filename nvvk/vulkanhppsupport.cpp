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

#include "vulkanhppsupport.hpp"
#include "memallocator_dedicated_vk.hpp"
#include "memorymanagement_vk.hpp"

namespace nvvk {
bool checkResult(vk::Result result, const char* message)
{
  return nvvk::checkResult(VkResult(result), message);
}

bool checkResult(vk::Result result, const char* file, int32_t line)
{
  return nvvk::checkResult((VkResult)result, file, line);
}
}

namespace nvvkpp {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExportResourceAllocator::ExportResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::MemAllocator* memAllocator, VkDeviceSize stagingBlockSize)
    : ResourceAllocator(device, physicalDevice, memAllocator, stagingBlockSize)
{
}

void ExportResourceAllocator::CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer)
{
  VkBufferCreateInfo               info = info_;
  VkExternalMemoryBufferCreateInfo infoEx{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO};
#ifdef WIN32
  infoEx.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
  infoEx.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
  info.pNext = &infoEx;
  NVVK_CHECK(vkCreateBuffer(m_device, &info, nullptr, buffer));
}

void ExportResourceAllocator::CreateImageEx(const VkImageCreateInfo& info_, VkImage* image)
{
  auto                            info = info_;
  VkExternalMemoryImageCreateInfo infoEx{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
#ifdef WIN32
  infoEx.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
  infoEx.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
  info.pNext = &infoEx;
  NVVK_CHECK(vkCreateImage(m_device, &info, nullptr, image));
}

nvvk::MemHandle ExportResourceAllocator::AllocateMemory(const nvvk::MemAllocateInfo& allocateInfo)
{
  nvvk::MemAllocateInfo exportAllocateInfo(allocateInfo);
  exportAllocateInfo.setExportable(true);
  return ResourceAllocator::AllocateMemory(exportAllocateInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ExportResourceAllocatorDedicated::ExportResourceAllocatorDedicated(VkDevice         device,
                                                                   VkPhysicalDevice physicalDevice,
                                                                   VkDeviceSize stagingBlockSize /*= NVVK_DEFAULT_STAGING_BLOCKSIZE*/)
{
  init(device, physicalDevice, stagingBlockSize);
}

ExportResourceAllocatorDedicated::~ExportResourceAllocatorDedicated()
{
  deinit();
}


void ExportResourceAllocatorDedicated::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize /*= NVVK_DEFAULT_STAGING_BLOCKSIZE*/)
{
  m_memAlloc = std::make_unique<nvvk::DedicatedMemoryAllocator>(device, physicalDevice);
  ExportResourceAllocator::init(device, physicalDevice, m_memAlloc.get(), stagingBlockSize);
}

void ExportResourceAllocatorDedicated::deinit()
{
  ExportResourceAllocator::deinit();
  m_memAlloc.reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExplicitDeviceMaskResourceAllocator::ExplicitDeviceMaskResourceAllocator(VkDevice            device,
                                                                         VkPhysicalDevice    physicalDevice,
                                                                         nvvk::MemAllocator* memAlloc,
                                                                         uint32_t            deviceMask)
{
  init(device, physicalDevice, memAlloc, deviceMask);
}

void ExplicitDeviceMaskResourceAllocator::init(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::MemAllocator* memAlloc, uint32_t deviceMask)
{
  ResourceAllocator::init(device, physicalDevice, memAlloc);
  m_deviceMask = deviceMask;
}

nvvk::MemHandle ExplicitDeviceMaskResourceAllocator::AllocateMemory(const nvvk::MemAllocateInfo& allocateInfo)
{
  nvvk::MemAllocateInfo deviceMaskAllocateInfo(allocateInfo);
  deviceMaskAllocateInfo.setDeviceMask(m_deviceMask);

  return ResourceAllocator::AllocateMemory(deviceMaskAllocateInfo);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ResourceAllocatorDma::ResourceAllocatorDma(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize, VkDeviceSize memBlockSize)
{
  init(device, physicalDevice, stagingBlockSize, memBlockSize);
}

ResourceAllocatorDma::~ResourceAllocatorDma()
{
  deinit();
}

void ResourceAllocatorDma::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize, VkDeviceSize memBlockSize)
{
  m_dma = std::make_unique<nvvk::DeviceMemoryAllocator>(device, physicalDevice, memBlockSize);
  ResourceAllocator::init(device, physicalDevice, m_dma.get(), stagingBlockSize);
}


void ResourceAllocatorDma::init(VkInstance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize, VkDeviceSize memBlockSize)
{
  init(device, physicalDevice, stagingBlockSize, memBlockSize);
}

void ResourceAllocatorDma::deinit()
{
  ResourceAllocator::deinit();
  m_dma.reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ResourceAllocatorDedicated::ResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize)
{
  init(device, physicalDevice, stagingBlockSize);
}


ResourceAllocatorDedicated::~ResourceAllocatorDedicated()
{
  deinit();
}

void ResourceAllocatorDedicated::init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize)
{
  m_memAlloc = std::make_unique<nvvk::DedicatedMemoryAllocator>(device, physicalDevice);
  ResourceAllocator::init(device, physicalDevice, m_memAlloc.get(), stagingBlockSize);
}


void ResourceAllocatorDedicated::init(VkInstance,  // unused
                                      VkDevice         device,
                                      VkPhysicalDevice physicalDevice,
                                      VkDeviceSize     stagingBlockSize /*= NVVK_DEFAULT_STAGING_BLOCKSIZE*/)
{
  init(device, physicalDevice, stagingBlockSize);
}

void ResourceAllocatorDedicated::deinit()
{
  ResourceAllocator::deinit();
  m_memAlloc.reset();
}

}  // namespace nvvkpp
