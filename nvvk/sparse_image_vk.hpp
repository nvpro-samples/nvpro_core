/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#pragma once
#include <array>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "nvmath/nvmath.h"
#include "nvvk/memorymanagement_vk.hpp"
#include "nvh/nvprint.hpp"
#include "nvh/container_utils.hpp"



// Mip level indexing relies on 32-bit unsigned integers
#define NVVK_SPARSE_IMAGE_MAX_MIP_LEVELS 32u

// Special error value used to catch indexing issues
#define NVVK_SPARSE_IMAGE_INVALID_INDEX (~0u)
namespace nvvk {

// Virtual texture page as a part of the partially resident texture
// Contains memory bindings, offsets and status information
struct SparseImagePage
{
  // Allocation flags to keep track of the next action
  // to take on the page memory
  enum AllocationFlagBits
  {
    // No action, keep the page in memory
    eNone = 0,
    // The page will have to be discarded once
    // no image references it
    eMarkedForDeletion = 1
  };
  typedef uint32_t AllocationFlags;

  // Offset in the mip level of the sparse texture, in texels
  VkOffset3D offset{};
  // Page extent, in texels
  VkExtent3D extent{};
  // Sparse image memory bind for this page
  VkSparseImageMemoryBind imageMemoryBind{};
  // Size in bytes of the page
  VkDeviceSize size{};
  // Mip level of the page
  uint32_t mipLevel{NVVK_SPARSE_IMAGE_INVALID_INDEX};
  // Layer the page belongs to
  uint32_t layer{NVVK_SPARSE_IMAGE_INVALID_INDEX};

  nvvk::MemHandle allocation;

  // Index of the page based on its location in the sparse texture
  // index = mipStartIndex + location.x + pageCount.x*(location.y + pageCount.z*location.z)
  // where mipStartIndex is the index of the first page of the mip level,
  // location is the 3D index of the page in the mip, and pageCount is the number of pages
  // of the mip in each dimension
  uint32_t index{0};

  // Application-managed timestamp, typically used for cache management
  uint32_t timeStamp{~0u};

  // Allocation flags for the page, either eNone for a page that is currently in use,
  // or eMarkedForDeletion, for pages that will be destroyed as soon as the sparse image
  // binding stops referencing them
  AllocationFlags allocationFlags{eNone};

  // Create the host-side data for the virtual page
  inline void bindDeviceMemory(VkDeviceMemory mem, VkDeviceSize memOffset)
  {
    imageMemoryBind.memoryOffset = memOffset;
    imageMemoryBind.memory       = mem;
  }

  inline bool hasBoundMemory() const { return imageMemoryBind.memory != VkDeviceMemory(); }
};

// Virtual texture object containing all pages
struct SparseImage
{
  // Number of VkImages referencing the sparse memory bindings
  // This allows updating the bindings of one image while
  // rendering with the other in another thread
  static const size_t s_sparseImageCount{2};

  // Texture image handles (see above)
  std::array<VkImage, s_sparseImageCount> images;
  // Index of the image that can be used for rendering
  uint32_t currentImage{0u};


  // Opaque memory bindings for the mip tail
  std::vector<VkSparseMemoryBind> opaqueMemoryBinds;
  // Memory allocation for the mip tail. This memory is allocated
  // upon creating the sparse image, and will remain allocated
  // even after a flush call
  std::vector<nvvk::MemHandle> mipTailAllocations;

  // Memory properties for the sparse texture allocations
  VkMemoryPropertyFlags memoryProperties{};

  // Sparse queue binding information
  VkBindSparseInfo bindSparseInfo{};

  // Memory bindings for virtual addressing
  std::vector<VkSparseImageMemoryBind> sparseImageMemoryBinds;

  // Page identifier, defined by its layer and its page index, which
  // is defined as mipStartIndex + location.x + pageCount.x*(location.y + pageCount.z*location.z)
  // where mipStartIndex is the index of the first page of the mip level,
  // location is the 3D index of the page in the mip, and pageCount is the number of pages
  // of the mip in each dimension
  struct PageId
  {
    uint32_t layer{};
    uint32_t page{};
  };


  // Storage for the currently allocated pages
  std::unordered_map<PageId, SparseImagePage, nvh::HashAligned32<PageId>, nvh::EqualMem<PageId>> allocatedPages;


  // Binding information for sparse texture pages
  VkSparseImageMemoryBindInfo imageMemoryBindInfo{};
  // Binding information for the mip tail
  VkSparseImageOpaqueMemoryBindInfo opaqueMemoryBindInfo{};

  // First mip level in mip tail
  uint32_t mipTailStart{NVVK_SPARSE_IMAGE_INVALID_INDEX};

  // Total sparse texture resolution
  VkExtent3D size{};
  // Number of possible mip levels of the image
  uint32_t mipLevelCount{NVVK_SPARSE_IMAGE_INVALID_INDEX};

  // Number of layers
  uint32_t layerCount{NVVK_SPARSE_IMAGE_INVALID_INDEX};

  // Memory requirements for page and mip tail allocations
  VkMemoryRequirements memoryReqs{};

  // Granularity of the image, representing the extent of the pages
  VkExtent3D imageGranularity{0u, 0u, 0u};

  // Get the number of pages currently allocated on the device
  size_t getAllocatedPageCount() const { return allocatedPages.size(); }

  // Get the image handle for rendering
  VkImage getCurrentImage() { return images[currentImage]; }
  // Get the image handle for update work
  VkImage getWorkImage() { return images[(currentImage + 1) % s_sparseImageCount]; }
  // Swap the current and work images
  void nextImage()
  {
    currentImage              = (currentImage + 1) % s_sparseImageCount;
    imageMemoryBindInfo.image = getWorkImage();
  }

  // Add mip tail information to the image, return the requested memory requirements for the mip tail
  VkMemoryRequirements addMipTail(VkMemoryRequirements             generalMemoryReqs,
                                  VkSparseImageMemoryRequirements& sparseMemoryReq,
                                  uint32_t                         layer = 0u);

  // Compute and store the number of pages contained in each mip level
  void computeMipPageCounts();
  // Create the sparse image, return the memory requirements for the mip tail(s)
  std::vector<VkMemoryRequirements> create(VkDevice                                      device,
                                           const std::array<VkImage, s_sparseImageCount> imageDesc,
                                           uint32_t                                      mipLevels,
                                           uint32_t                                      arrayLayers,
                                           const VkExtent3D&                             extent);
  // Bind device memory to the mip tail(s)
  void bindMipTailMemory(std::vector<std::pair<VkDeviceMemory, VkDeviceSize>> mipTailMemory);

  // Unbind device memory from the mip tail(s)
  void unbindMipTailMemory();
  // Add a page to the sparse image
  void addPage(VkImageSubresource subresource, VkOffset3D offset, VkExtent3D extent, const VkDeviceSize size, const uint32_t mipLevel, uint32_t layer);


  // Update the contents of sparseImageMemoryBinds based on the vector of updated page indices and
  // set the pointers in the VkBindSparseInfo
  // Call before sparse binding to update memory bind list etc.
  // No synchronization is added to the VkBindSparseInfo object, the application
  // is responsible for adding the proper semaphore before calling vkQueueBindSparse
  void updateSparseBindInfo(const std::vector<uint32_t>& updatedPageIndices, uint32_t layer = 0);

  // Set the pointers in the VkBindSparseInfo using the contents of sparseImageMemoryBinds
  // No synchronization is added to the VkBindSparseInfo object, the application
  // is responsible for adding the proper semaphore before calling vkQueueBindSparse
  void updateSparseBindInfo();

  // Get the index of the beginning of a mip level in the page list
  uint32_t mipStartIndex(uint32_t mipLevel);

  // Compute the index of a page within a mip level in the page list
  inline uint32_t indexInMip(const SparseImagePage& p)
  {
    nvmath::vec3ui mipSize(std::max(size.width >> p.mipLevel, 1u), std::max(size.height >> p.mipLevel, 1u),
                           std::max(size.depth >> p.mipLevel, 1u));
    nvmath::vec3ui location(p.offset.x / mipSize.x, p.offset.y / mipSize.y, p.offset.z / mipSize.z);
    uint32_t       pageWidth  = p.extent.width;
    uint32_t       pageHeight = std::max(1u, p.extent.height);
    if(pageWidth == 0 || pageHeight == 0)
    {
      LOGE("indexInMip: Invalid page dimensions");
      assert(false);
      return NVVK_SPARSE_IMAGE_INVALID_INDEX;
    }
    uint32_t index = location.x + (mipSize.x / pageWidth) * (location.y + location.z * (mipSize.y / pageHeight));
    return index;
  }

  // Compute the index of a page in the page list
  inline uint32_t pageIndex(const SparseImagePage& p)
  {
    uint32_t index = indexInMip(p);
    return pageIndex(p.mipLevel, index);
  }

  // Compute the index of a page in the page list based on its mip level and index within
  // that mip level
  inline uint32_t pageIndex(uint32_t mipLevel, uint32_t indexInMip)
  {
    uint32_t mipStart = mipStartIndex(mipLevel);
    if(mipStart == NVVK_SPARSE_IMAGE_INVALID_INDEX)
    {
      LOGE("pageIndex: invalid mip level");
      assert(false);
      return NVVK_SPARSE_IMAGE_INVALID_INDEX;
    }

    if(indexInMip == NVVK_SPARSE_IMAGE_INVALID_INDEX)
    {
      LOGE("pageIndex: cannot find page index in mip level");
      assert(false);
      return NVVK_SPARSE_IMAGE_INVALID_INDEX;
    }
    return mipStart + indexInMip;
  }


  // Compute the indices of the children of a page, representing the same area of the image at a finer mip level
  std::vector<uint32_t> pageChildIndices(const SparseImagePage& p);

  // Create the page information from its page index and layer
  SparseImagePage createPageInfo(uint32_t pageIndex, uint32_t layer);


private:
  // Start index of each mip level
  std::vector<uint32_t> sparseMipStartIndices;
  // Number of pages in each mip level
  std::vector<uint32_t> sparseMipPageCounts;
  // Total page count for the sparse image
  uint32_t sparseMipTotalPageCount{};
};


}  // namespace nvvk
