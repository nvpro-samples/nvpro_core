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


#include "sparse_image_vk.hpp"

// Compute the number of pages of size `granularity` would be required to represent a texture of size `extent`
static inline nvmath::vec3ui alignedDivision(const VkExtent3D& extent, const VkExtent3D& granularity)
{
  nvmath::vec3ui res;
  if(granularity.width == 0 || granularity.height == 0 || granularity.depth == 0)
  {
    LOGE("alignedDivision: invalid granularity\n");
    assert(false);
    return nvmath::vec3ui(0u);
  }
  res.x = (extent.width + granularity.width - 1) / granularity.width;
  res.y = (extent.height + granularity.height - 1) / granularity.height;
  res.z = (extent.depth + granularity.depth - 1) / granularity.depth;
  return res;
}

// Set the pointers for the VkBindSparseInfo stored in `image` prior to
// calling vkQueueBindSparse
void applySparseMemoryBinds(nvvk::SparseImage& image)
{
  image.bindSparseInfo = VkBindSparseInfo{VK_STRUCTURE_TYPE_BIND_SPARSE_INFO};

  // Sparse Image memory binds
  image.imageMemoryBindInfo.image     = image.getWorkImage();
  image.imageMemoryBindInfo.bindCount = static_cast<uint32_t>(image.sparseImageMemoryBinds.size());
  image.imageMemoryBindInfo.pBinds    = image.sparseImageMemoryBinds.data();
  image.bindSparseInfo.imageBindCount = ((image.imageMemoryBindInfo.bindCount > 0) ? 1 : 0);
  image.bindSparseInfo.pImageBinds    = &image.imageMemoryBindInfo;

  // Opaque image memory binds (mip tail)
  image.opaqueMemoryBindInfo.image          = image.getWorkImage();
  image.opaqueMemoryBindInfo.bindCount      = static_cast<uint32_t>(image.opaqueMemoryBinds.size());
  image.opaqueMemoryBindInfo.pBinds         = image.opaqueMemoryBinds.data();
  image.bindSparseInfo.imageOpaqueBindCount = ((image.opaqueMemoryBindInfo.bindCount > 0) ? 1 : 0);
  image.bindSparseInfo.pImageOpaqueBinds    = &image.opaqueMemoryBindInfo;
}

// Add mip tail information to the image, return the requested memory requirements for the mip tail
VkMemoryRequirements nvvk::SparseImage::addMipTail(VkMemoryRequirements             generalMemoryReqs,
                                                   VkSparseImageMemoryRequirements& sparseMemoryReq,
                                                   uint32_t                         layer /*= 0*/)
{
  // Compute the size of the required mip tail allocation
  VkMemoryRequirements memReqs = generalMemoryReqs;
  memReqs.size                 = sparseMemoryReq.imageMipTailSize;

  // Add an `opaque` memory bind representing the mip tail
  VkSparseMemoryBind sparseMemoryBind{sparseMemoryReq.imageMipTailOffset + layer * sparseMemoryReq.imageMipTailStride,
                                      sparseMemoryReq.imageMipTailSize, VK_NULL_HANDLE};
  opaqueMemoryBinds.push_back(sparseMemoryBind);
  // Return the memory requirements for that mip tail
  return memReqs;
}

// Compute and store the number of pages contained in each sparse mip level
void nvvk::SparseImage::computeMipPageCounts()
{
  uint32_t dimensionCount = 1;
  if(size.height > 1)
  {
    dimensionCount++;
  }
  if(size.depth > 1)
  {
    dimensionCount++;
  }

  // Since the finest mip level has index 0, the number
  // of sparse levels is equal to the index of the beginning of the
  // mip tail
  uint32_t sparseMipLevels = mipTailStart;
  sparseMipStartIndices.resize(sparseMipLevels);
  sparseMipPageCounts.resize(sparseMipLevels);

  // Compute the page count at the coarsest sparse level (just above the mip tail)
  // For each dimension we compare the resolution of the mip level with the page granularity and
  // keep the highest ration. This is particularly necessary for 3D textures, where the depth
  // granularity is typically lower than the width and height granularities
  uint32_t pageCountAtCoarsestLevel = (size.width >> (sparseMipLevels - 1)) / imageGranularity.width;
  pageCountAtCoarsestLevel =
      std::max(pageCountAtCoarsestLevel,
               pageCountAtCoarsestLevel * ((size.height >> (sparseMipLevels - 1)) / imageGranularity.height));
  pageCountAtCoarsestLevel =
      std::max(pageCountAtCoarsestLevel,
               pageCountAtCoarsestLevel * (size.depth >> (sparseMipLevels - 1)) / imageGranularity.depth);


  // When going from level n+1 to level n each dimension will
  // be divided by 2, hence each page at level n+1 will be represented
  // by 2^dimensionCount children at level n
  uint32_t childCount = 1 << dimensionCount;


  // The indices of the pages start from the coarsest level, so the
  // first page of that level will have index 0, and the pages of the
  // finest level will have the highest indices
  uint32_t finalIndex          = 0;
  uint32_t currentPagesInLevel = pageCountAtCoarsestLevel;
  uint32_t currentMipLevel     = sparseMipLevels - 1;

  sparseMipTotalPageCount = 0u;
  // Iterate from coarsest to finest level, accumulating the
  // page counts for each level
  for(uint32_t i = 0; i < sparseMipLevels; i++, currentMipLevel--)
  {
    sparseMipStartIndices[currentMipLevel] = finalIndex;
    sparseMipPageCounts[currentMipLevel]   = currentPagesInLevel;
    finalIndex += currentPagesInLevel;
    currentPagesInLevel *= childCount;
  }
  sparseMipTotalPageCount = finalIndex;
}

// Create the sparse image, return the memory requirements for the mip tail(s)
std::vector<VkMemoryRequirements> nvvk::SparseImage::create(VkDevice                                      device,
                                                            const std::array<VkImage, s_sparseImageCount> imageDesc,
                                                            uint32_t                                      mipLevels,
                                                            uint32_t                                      arrayLayers,
                                                            const VkExtent3D&                             extent)
{
  if(mipLevels > NVVK_SPARSE_IMAGE_MAX_MIP_LEVELS)
  {
    LOGE("SparseImage::create: invalid mip level count\n");
    assert(false);
    return {};
  }

  std::vector<VkMemoryRequirements> mipTailRequirements;

  // Create the image descriptor
  size.width  = extent.width;
  size.height = extent.height;
  size.depth  = extent.depth;
  images      = imageDesc;


  mipLevelCount = mipLevels;
  layerCount    = arrayLayers;

  // Get memory requirements for later allocations
  vkGetImageMemoryRequirements(device, images[0], &memoryReqs);

  // Get sparse memory requirements
  std::vector<VkSparseImageMemoryRequirements> sparseMemoryReqs;
  uint32_t                                     reqCount = 0u;
  vkGetImageSparseMemoryRequirements(device, images[0], &reqCount, nullptr);

  if(reqCount == 0u)
  {
    LOGE("No sparse image memory requirements available\n");
    return {};
  }

  sparseMemoryReqs.resize(reqCount);
  vkGetImageSparseMemoryRequirements(device, images[0], &reqCount, sparseMemoryReqs.data());


  // Select the memory requirements with the smallest granularity to avoid wasting memory
  uint32_t                        minGranularity = NVVK_SPARSE_IMAGE_INVALID_INDEX;
  VkSparseImageMemoryRequirements sparseReqs     = {};
  for(const auto& reqs : sparseMemoryReqs)
  {
    uint32_t granularity = reqs.formatProperties.imageGranularity.width * reqs.formatProperties.imageGranularity.height
                           * reqs.formatProperties.imageGranularity.depth;
    if(granularity < minGranularity)
    {
      minGranularity = granularity;
      sparseReqs     = reqs;
    }
  }

  // sparseMemoryReq.imageMipTailFirstLod is the first mip level stored inside the mip tail
  mipTailStart = sparseReqs.imageMipTailFirstLod;

  // Get sparse image memory requirements for the color aspect
  VkSparseImageMemoryRequirements sparseMemoryReq;
  bool                            colorAspectFound = false;
  for(const auto& reqs : sparseMemoryReqs)
  {
    if((reqs.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0)
    {
      sparseMemoryReq  = reqs;
      colorAspectFound = true;
      break;
    }
  }
  if(!colorAspectFound)
  {
    LOGE("Could not find sparse image memory requirements with color aspect bit");
    return {};
  }

  // Check whether a mip tail is necessary
  bool hasMipTail = (sparseMemoryReq.imageMipTailFirstLod < mipLevels);
  // Check if the format has a single mip tail for all layers or one mip tail for each layer
  // The mip tail contains all mip levels >= sparseMemoryReq.imageMipTailFirstLod
  bool singleMipTail = ((sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0);


  imageGranularity = sparseMemoryReq.formatProperties.imageGranularity;

  // Prepare the data structure holding all the virtual pages for the sparse texture

  // Sparse bindings for each mip level of each layer, excepting the mip levels of the mip tail
  for(uint32_t layer = 0; layer < arrayLayers; layer++)
  {
    // If the format has one mip tail per layer, allocate each of them on the device
    if((!singleMipTail) && hasMipTail)
    {
      mipTailRequirements.push_back(addMipTail(memoryReqs, sparseMemoryReq, layer));
    }
  }


  // If the format has a single mip tail for all layers, allocate it on the device
  if(singleMipTail && hasMipTail)
  {
    mipTailRequirements.push_back(addMipTail(memoryReqs, sparseMemoryReq));
  }

  // Compute the page indices for each mip level
  computeMipPageCounts();

  return mipTailRequirements;
}

// Bind device memory to the mip tail(s)
void nvvk::SparseImage::bindMipTailMemory(std::vector<std::pair<VkDeviceMemory, VkDeviceSize>> mipTailMemory)
{
  if(mipTailMemory.size() != opaqueMemoryBinds.size())
  {
    LOGE("Mip tail allocations count must match the number of mip tails in the sparse texture\n");
    return;
  }
  for(size_t i = 0; i < mipTailMemory.size(); i++)
  {
    opaqueMemoryBinds[i].memory       = mipTailMemory[i].first;
    opaqueMemoryBinds[i].memoryOffset = mipTailMemory[i].second;
  }
}

// Unbind device memory from the mip tail(s)
void nvvk::SparseImage::unbindMipTailMemory()
{
  for(size_t i = 0; i < opaqueMemoryBinds.size(); i++)
  {
    opaqueMemoryBinds[i].resourceOffset = {};
    opaqueMemoryBinds[i].memory         = {};
    opaqueMemoryBinds[i].memoryOffset   = {};
  }
}

// Update the contents of sparseImageMemoryBinds based on the vector of updated page indices and
// set the pointers in the VkBindSparseInfo
// Call before sparse binding to update memory bind list etc.
// No synchronization is added to the VkBindSparseInfo object, the application
// is responsible for adding the proper semaphore before calling vkQueueBindSparse
void nvvk::SparseImage::updateSparseBindInfo(const std::vector<uint32_t>& updatedPageIndices, uint32_t layer /*= 0*/)
{
  // Build the list of added/removed sparse image memory binds
  sparseImageMemoryBinds.resize(updatedPageIndices.size());
  uint32_t index = 0;

  for(auto pageIndex : updatedPageIndices)
  {
    PageId id = {layer, pageIndex};
    auto   it = allocatedPages.find(id);
    // If the page actually exists in the image and is not flagged for deletion,
    // add it to the bindings
    if(it != allocatedPages.end() && (it->second.allocationFlags & SparseImagePage::eMarkedForDeletion) == 0)
    {
      const auto& page              = it->second;
      sparseImageMemoryBinds[index] = page.imageMemoryBind;
      index++;
    }
    else
    {
      // Otherwise the page has been deleted, and the sparse texture bindings
      // are updated by binding VK_NULL_HANDLE memory to the page
      SparseImagePage page          = createPageInfo(pageIndex, layer);
      sparseImageMemoryBinds[index] = page.imageMemoryBind;
      index++;
    }
  }
  // Set the pointers before calling vkQueueBindSparse
  applySparseMemoryBinds(*this);
}

// Set the pointers in the VkBindSparseInfo using the contents of sparseImageMemoryBinds
// No synchronization is added to the VkBindSparseInfo object, the application
// is responsible for adding the proper semaphore before calling vkQueueBindSparse
void nvvk::SparseImage::updateSparseBindInfo()
{
  // Set the pointers before calling vkQueueBindSparse
  applySparseMemoryBinds(*this);
}

// Get the index of the beginning of a mip level in the page list
uint32_t nvvk::SparseImage::mipStartIndex(uint32_t mipLevel)
{
  return sparseMipStartIndices[mipLevel];
}

// Compute the indices of the children of a page, representing the same area of the image at a finer mip level
std::vector<uint32_t> nvvk::SparseImage::pageChildIndices(const SparseImagePage& p)
{
  std::vector<uint32_t> res(p.extent.depth <= 1 ? 4 : 8, NVVK_SPARSE_IMAGE_INVALID_INDEX);

  if(p.mipLevel == 0)
  {
    return res;
  }

  if(p.extent.width == 0u || p.extent.height == 0u || p.extent.depth == 0u)
  {
    LOGE("pageChildIndices: Invalid page extent");
    assert(false);
    return res;
  }

  // Get the index from which the pages of the next mip level
  // are defined, and sanity check the result
  uint32_t mipStart = mipStartIndex(p.mipLevel - 1);
  if(mipStart == NVVK_SPARSE_IMAGE_INVALID_INDEX)
  {
    LOGE("pageChildIndices: Invalid mip start index");
    assert(false);
    return res;
  }

  // Compute the size of the child mip level in texels, defined by originalSize/(2^level)
  nvmath::vec3ui mipSize(std::max(size.width >> (p.mipLevel - 1), 1u), std::max(size.height >> (p.mipLevel - 1), 1u),
                         std::max(size.depth >> (p.mipLevel - 1), 1u));
  // Compute the location of the beginning of the child list in the next mip level, where each dimension contains
  // twice as many pages as the parent level
  nvmath::vec3ui location(2 * p.offset.x / p.extent.width, 2 * p.offset.y / p.extent.height, 2 * p.offset.z / p.extent.depth);


  uint32_t pageWidth  = p.extent.width;
  uint32_t pageHeight = p.extent.height;
  uint32_t pageDepth  = std::max(1u, p.extent.depth);

  // Number of pages along one row (X) of the texture, and within one slice (X*Y) of the texture
  uint32_t pagesPerRow   = (mipSize.x / pageWidth);
  uint32_t pagesPerSlice = (mipSize.x * mipSize.y) / (pageWidth * pageHeight);

  // Build and return the child list
  for(uint32_t z = 0; z < (pageDepth > 1 ? 2u : 1u); z++)
  {
    for(uint32_t y = 0; y < (pageHeight > 1 ? 2u : 1u); y++)
    {
      for(uint32_t x = 0; x < 2; x++)
      {
        res[x + 2 * (y + 2 * z)] = (location.z + z) * pagesPerSlice + mipStart + location.x + x + (location.y + y) * pagesPerRow;
      }
    }
  }
  return res;
}

// Create the page information from its page index and layer
nvvk::SparseImagePage nvvk::SparseImage::createPageInfo(uint32_t pageIndex, uint32_t layer)
{
  uint32_t dimensionCount = 1;
  if(size.height != 0)
    dimensionCount++;
  if(size.depth != 0)
    dimensionCount++;

  std::vector<uint32_t>& startIndices = sparseMipStartIndices;

  // Find the mip level from the global page index by comparing the
  // start indices of the mip levels with the page index
  // There are at most 32 levels (including the mip tail)
  // so a linear search is fast enough
  uint32_t mipLevel = 0;
  for(size_t i = 0; i < startIndices.size(); i++)
  {
    size_t currentMipLevel = startIndices.size() - i - 1;
    if(pageIndex >= startIndices[currentMipLevel] && (currentMipLevel == 0 || pageIndex < startIndices[currentMipLevel - 1]))
    {
      mipLevel = uint32_t(currentMipLevel);
      break;
    }
  }
  // Get the local index of the page within its mip level
  uint32_t indexInMip = pageIndex - startIndices[mipLevel];

  // Resolution of the mip level, defined by the sparse image total size / 2^level
  VkExtent3D mipResolution{std::max(size.width >> mipLevel, 1u), std::max(size.height >> mipLevel, 1u),
                           std::max(size.depth >> mipLevel, 1u)};

  // Compute the number of pages required in each dimension for the mip level
  nvmath::vec3ui sparseBindCounts = alignedDivision(mipResolution, imageGranularity);

  // Compute the page index in each dimension and deduce the offset of the page
  // in texels based on the page granularity
  uint32_t   x = indexInMip % sparseBindCounts.x;
  uint32_t   y = (indexInMip / sparseBindCounts.x) % sparseBindCounts.y;
  uint32_t   z = indexInMip / (sparseBindCounts.x * sparseBindCounts.y);
  VkOffset3D offset{int32_t(x * imageGranularity.width), int32_t(y * imageGranularity.height),
                    int32_t(z * imageGranularity.depth)};

  // Compute the size of the last page on each dimension in the case the image has non-power-of-two dimension
  nvmath::vec3ui lastBlockExtent;
  lastBlockExtent.x = (mipResolution.width % imageGranularity.width) ? mipResolution.width % imageGranularity.width :
                                                                       imageGranularity.width;
  lastBlockExtent.y = (mipResolution.height % imageGranularity.height) ? mipResolution.height % imageGranularity.height :
                                                                         imageGranularity.height;
  lastBlockExtent.z = (mipResolution.depth % imageGranularity.depth) ? mipResolution.depth % imageGranularity.depth :
                                                                       imageGranularity.depth;


  // Size of the page, including the nonuniform size on the edges of the image
  VkExtent3D pageSize{(x == sparseBindCounts.x - 1) ? lastBlockExtent.x : imageGranularity.width,
                      (y == sparseBindCounts.y - 1) ? lastBlockExtent.y : imageGranularity.height,
                      (z == sparseBindCounts.z - 1) ? lastBlockExtent.z : imageGranularity.depth};

  // Set and return the page information, with empty memory allocation
  VkImageSubresource subresource{VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, layer};

  SparseImagePage newPage{};
  newPage.offset                      = offset;
  newPage.extent                      = pageSize;
  newPage.size                        = memoryReqs.alignment;
  newPage.mipLevel                    = mipLevel;
  newPage.layer                       = layer;
  newPage.imageMemoryBind.offset      = offset;
  newPage.imageMemoryBind.extent      = pageSize;
  newPage.imageMemoryBind.subresource = subresource;
  newPage.index                       = pageIndex;
  newPage.allocationFlags             = SparseImagePage::eNone;
  newPage.timeStamp                   = ~0u;
  return newPage;
}
