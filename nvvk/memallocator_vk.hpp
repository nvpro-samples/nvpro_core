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

#include <vulkan/vulkan_core.h>

#include <string>

namespace nvvk {
class MemHandleBase;
typedef MemHandleBase* MemHandle;
static const MemHandle NullMemHandle = nullptr;


/**
  \class nvvk::MemHandle

  nvvk::MemHandle represents a memory allocation or sub-allocation from the
  generic nvvk::MemAllocator interface. Ideally use `nvvk::NullMemHandle` for
  setting to 'NULL'. MemHandle may change to a non-pointer type in future.

  \class nvvk::MemAllocateInfo

  nvvk::MemAllocateInfo is collecting almost all parameters a Vulkan allocation could potentially need.
  This keeps MemAllocator's interface simple and extensible.
*/

class MemAllocateInfo
{
public:
  MemAllocateInfo(const VkMemoryRequirements& memReqs,  // determine size, alignment and memory type
                  VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,  // determine device_local, host_visible, host coherent etc...
                  bool isTilingOptimal = false  // determine if the alocation is going to be used for an VK_IMAGE_TILING_OPTIMAL image
  );

  // Convenience constructures that infer the allocation information from the buffer object directly
  MemAllocateInfo(VkDevice device, VkBuffer buffer, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  // Convenience constructures that infer the allocation information from the image object directly.
  // If the driver _prefers_ a dedicated allocation for this particular image and allowDedicatedAllocation is true, a dedicated allocation will be requested.
  // If the driver _requires_ a dedicated allocation, a dedicated allocation will be requested regardless of 'allowDedicatedAllocation'.
  MemAllocateInfo(VkDevice device, VkImage image, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool allowDedicatedAllocation = true);

  // Determines which heap to allocate from
  MemAllocateInfo& setMemoryProperties(VkMemoryPropertyFlags flags);
  // Determines size and alignment
  MemAllocateInfo& setMemoryRequirements(VkMemoryRequirements requirements);
  // TilingOptimal should be set for images. The allocator may choose to separate linear and tiling allocations
  MemAllocateInfo& setTilingOptimal(bool isTilingOptimal);
  // The allocation will be dedicated for the given image
  MemAllocateInfo& setDedicatedImage(VkImage image);
  // The allocation will be dedicated for the given buffer
  MemAllocateInfo& setDedicatedBuffer(VkBuffer buffer);
  // Set additional allocation flags
  MemAllocateInfo& setAllocationFlags(VkMemoryAllocateFlags flags);
  // Set the device mask for the allocation, redirect allocations to specific device(s) in the device group
  MemAllocateInfo& setDeviceMask(uint32_t mask);
  // Set a name for the allocation (only useful for dedicated allocations or allocators)
  MemAllocateInfo& setDebugName(const std::string& name);
  // Make the allocation exportable
  MemAllocateInfo& setExportable(bool exportable);
  // Prioritize the allocation (values 0.0 - 1.0); this may guide eviction strategies
  MemAllocateInfo& setPriority(const float priority = 0.5f);

  VkImage                      getDedicatedImage() const { return m_dedicatedImage; }
  VkBuffer                     getDedicatedBuffer() const { return m_dedicatedBuffer; }
  VkMemoryAllocateFlags        getAllocationFlags() const { return m_allocateFlags; }
  uint32_t                     getDeviceMask() const { return m_deviceMask; }
  bool                         getTilingOptimal() const { return m_isTilingOptimal; }
  const VkMemoryRequirements&  getMemoryRequirements() const { return m_memReqs; }
  const VkMemoryPropertyFlags& getMemoryProperties() const { return m_memProps; }
  std::string                  getDebugName() const { return m_debugName; }
  bool                         getExportable() const { return m_isExportable; }
  float                        getPriority() const { return m_priority; }


private:
  VkBuffer              m_dedicatedBuffer{VK_NULL_HANDLE};
  VkImage               m_dedicatedImage{VK_NULL_HANDLE};
  VkMemoryAllocateFlags m_allocateFlags{0};
  uint32_t              m_deviceMask{0};
  VkMemoryRequirements  m_memReqs{0, 0, 0};
  VkMemoryPropertyFlags m_memProps{0};
  float                 m_priority{0.5f};

  std::string m_debugName;

  bool m_isTilingOptimal{false};
  bool m_isExportable{false};
};

// BakedAllocateInfo is a group of allocation relevant Vulkan allocation structures,
// which will be filled out and linked via pNext-> to be used directly via vkAllocateMemory.
struct BakedAllocateInfo
{
  BakedAllocateInfo() = default;

  // In lieu of proper copy operators, need to delete them as we store
  // addresses to members in other members. Copying such object would make them point to
  // wrong or out-of-scope addresses
  BakedAllocateInfo(BakedAllocateInfo&& other) = delete;
  BakedAllocateInfo operator=(BakedAllocateInfo&& other) = delete;
  BakedAllocateInfo(const BakedAllocateInfo&)            = delete;
  BakedAllocateInfo operator=(const BakedAllocateInfo) = delete;

  VkMemoryAllocateInfo          memAllocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  VkMemoryAllocateFlagsInfo     flagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
  VkMemoryDedicatedAllocateInfo dedicatedInfo{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
  VkExportMemoryAllocateInfo    exportInfo{VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
};

bool     fillBakedAllocateInfo(const VkPhysicalDeviceMemoryProperties& physMemProps, const MemAllocateInfo& info, BakedAllocateInfo& baked);
uint32_t getMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, const VkMemoryPropertyFlags& properties);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
  \class nvvk::MemAllocator

 nvvk::MemAllocator is a Vulkan memory allocator interface extensively used by ResourceAllocator.
 It provides means to allocate, free, map and unmap pieces of Vulkan device memory.
 Concrete implementations derive from nvvk::MemoryAllocator.
 They can implement the allocator dunctionality themselves or act as an adapter to another
 memory allocator implementation.

 A nvvk::MemAllocator hands out opaque 'MemHandles'. The implementation of the MemAllocator interface
 may chose any type of payload to store in a MemHandle. A MemHandle's relevant information can be 
 retrieved via getMemoryInfo().
*/
class MemAllocator
{
public:
  struct MemInfo
  {
    VkDeviceMemory memory;
    VkDeviceSize   offset;
    VkDeviceSize   size;
  };

  // Allocate a piece of memory according to the requirements of allocInfo.
  // may return NullMemHandle on error (provide pResult for details)
  virtual MemHandle allocMemory(const MemAllocateInfo& allocInfo, VkResult* pResult = nullptr) = 0;

  // Free the memory backing 'memHandle'.
  // memHandle may be nullptr;
  virtual void freeMemory(MemHandle memHandle) = 0;

  // Retrieve detailed information about 'memHandle'
  virtual MemInfo getMemoryInfo(MemHandle memHandle) const = 0;

  // Maps device memory to system memory.
  // If 'memHandle' already refers to a suballocation 'offset' will be applied on top of the
  // suballocation's offset inside the device memory.
  // may return nullptr on error (provide pResult for details)
  virtual void* map(MemHandle memHandle, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE, VkResult* pResult = nullptr) = 0;

  // Unmap memHandle
  virtual void unmap(MemHandle memHandle) = 0;

  // Convenience function to allow mapping straight to a typed pointer.
  template <class T>
  T* mapT(MemHandle memHandle, VkResult* pResult = nullptr)
  {
    return (T*)map(memHandle, 0, VK_WHOLE_SIZE, pResult);
  }

  virtual VkDevice         getDevice() const         = 0;
  virtual VkPhysicalDevice getPhysicalDevice() const = 0;

  // Make sure the dtor is virtual
  virtual ~MemAllocator() = default;
};

// Base class for memory handles
// Individual allocators will derive from it and fill the handles with their own data.
class MemHandleBase
{
public:
  virtual ~MemHandleBase() = default;  // force the class to become virtual
};

}  // namespace nvvk
