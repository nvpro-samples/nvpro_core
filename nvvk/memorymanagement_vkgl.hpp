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


#if NVP_SUPPORTS_OPENGL

/**
  This file contains helpers for resource interoperability between OpenGL and Vulkan.
  they only exist if the nvpro_core project is compiled with Vulkan AND OpenGL support.
*/


#pragma once

#include <nvgl/extensions_gl.hpp>
#include <nvvk/images_vk.hpp>
#include <nvvk/memorymanagement_vk.hpp>
#include <vulkan/vulkan_core.h>

namespace nvvk {

struct AllocationGL
{
  GLuint   memoryObject = 0;
  GLuint64 offset       = 0;
  GLuint64 size         = 0;
};

//////////////////////////////////////////////////////////////////////////

/** 
  \class nvvk::DeviceMemoryAllocatorGL

  nvvk::DeviceMemoryAllocatorGL is derived from nvvk::DeviceMemoryAllocator it uses vulkan memory that is exported
  and directly imported into OpenGL. Requires GL_EXT_memory_object.

  Used just like the original class however a new function to get the 
  GL memory object exists: `getAllocationGL`.

  Look at source of nvvk::AllocatorDmaGL for usage.
*/


class DeviceMemoryAllocatorGL : public DeviceMemoryAllocator
{
public:
  DeviceMemoryAllocatorGL() {}
  DeviceMemoryAllocatorGL(VkDevice         device,
                          VkPhysicalDevice physicalDevice,
                          VkDeviceSize     blockSize = NVVK_DEFAULT_MEMORY_BLOCKSIZE,
                          VkDeviceSize     maxSize   = 0)
      : DeviceMemoryAllocator(device, physicalDevice, blockSize, maxSize)
  {
  }


  AllocationGL getAllocationGL(AllocationID aid) const
  {
    AllocationGL          alloc;
    const AllocationInfo& info = getInfo(aid);
    alloc.memoryObject         = m_blockGLs[info.block.index].memoryObject;
    alloc.offset               = info.allocation.offset;
    alloc.size                 = info.allocation.size;
    return alloc;
  }

  static VkExternalMemoryHandleTypeFlags getExternalMemoryHandleTypeFlags();

protected:
  struct BlockGL
  {
#ifdef WIN32
    void* handle = nullptr;
#else
    int handle = -1;
#endif
    GLuint memoryObject = 0;
  };

  std::vector<BlockGL> m_blockGLs;

  struct StructChain
  {
    VkStructureType    sType;
    const StructChain* pNext;
  };

  VkResult allocBlockMemory(BlockID id, VkMemoryAllocateInfo& memInfo, VkDeviceMemory& deviceMemory) override;
  void     freeBlockMemory(BlockID id, VkDeviceMemory deviceMemory) override;
  void     resizeBlocks(uint32_t count) override;

  VkResult createBufferInternal(VkDevice device, const VkBufferCreateInfo* info, VkBuffer* buffer) override;
  VkResult createImageInternal(VkDevice device, const VkImageCreateInfo* info, VkImage* image) override;
};
}  // namespace nvvk
#endif
