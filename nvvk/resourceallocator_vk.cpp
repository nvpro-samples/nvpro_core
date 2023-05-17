/*
 * Copyright (c) 2019-2023, NVIDIA CORPORATION.  All rights reserved.
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

#include "resourceallocator_vk.hpp"

#include "memallocator_dma_vk.hpp"
#include "memallocator_dedicated_vk.hpp"
#include "error_vk.hpp"
#include "images_vk.hpp"

namespace nvvk {

ResourceAllocator::ResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, VkDeviceSize stagingBlockSize)
{
  init(device, physicalDevice, memAlloc, stagingBlockSize);
}

ResourceAllocator::~ResourceAllocator()
{
  deinit();
}

void ResourceAllocator::init(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, VkDeviceSize stagingBlockSize)
{
  m_device         = device;
  m_physicalDevice = physicalDevice;
  m_memAlloc       = memAlloc;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);
  m_samplerPool.init(device);
  m_staging = std::make_unique<StagingMemoryManager>(memAlloc, stagingBlockSize);
}

void ResourceAllocator::deinit()
{
  m_samplerPool.deinit();
  m_staging.reset();
}

Buffer ResourceAllocator::createBuffer(const VkBufferCreateInfo& info_, const VkMemoryPropertyFlags memProperties_)
{
  Buffer resultBuffer;
  // Create Buffer (can be overloaded)
  CreateBufferEx(info_, &resultBuffer.buffer);

  // Find memory requirements
  VkMemoryRequirements2           memReqs{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements   dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkBufferMemoryRequirementsInfo2 bufferReqs{VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};

  memReqs.pNext     = &dedicatedRegs;
  bufferReqs.buffer = resultBuffer.buffer;

  vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);

  // Build up allocation info
  MemAllocateInfo allocInfo(memReqs.memoryRequirements, memProperties_, false);

  if(info_.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
  {
    allocInfo.setAllocationFlags(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
  }
  if(dedicatedRegs.requiresDedicatedAllocation)
  {
    allocInfo.setDedicatedBuffer(resultBuffer.buffer);
  }

  // Allocate memory
  resultBuffer.memHandle = AllocateMemory(allocInfo);
  if(resultBuffer.memHandle)
  {
    const auto memInfo = m_memAlloc->getMemoryInfo(resultBuffer.memHandle);
    // Bind memory to buffer
    NVVK_CHECK(vkBindBufferMemory(m_device, resultBuffer.buffer, memInfo.memory, memInfo.offset));
  }
  else
  {
    destroy(resultBuffer);
  }

  return resultBuffer;
}

Buffer ResourceAllocator::createBuffer(VkDeviceSize size_, VkBufferUsageFlags usage_, const VkMemoryPropertyFlags memUsage_)
{
  VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  info.size  = size_;
  info.usage = usage_ | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  return createBuffer(info, memUsage_);
}

Buffer ResourceAllocator::createBuffer(const VkCommandBuffer& cmdBuf,
                                       const VkDeviceSize&    size_,
                                       const void*            data_,
                                       VkBufferUsageFlags     usage_,
                                       VkMemoryPropertyFlags  memProps)
{
  VkBufferCreateInfo createInfoR{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  createInfoR.size    = size_;
  createInfoR.usage   = usage_ | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  Buffer resultBuffer = createBuffer(createInfoR, memProps);

  if(data_)
  {
    m_staging->cmdToBuffer(cmdBuf, resultBuffer.buffer, 0, size_, data_);
  }

  return resultBuffer;
}

Image ResourceAllocator::createImage(const VkImageCreateInfo& info_, const VkMemoryPropertyFlags memUsage_)
{
  Image resultImage;
  // Create image
  CreateImageEx(info_, &resultImage.image);

  // Find memory requirements
  VkMemoryRequirements2          memReqs{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements  dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkImageMemoryRequirementsInfo2 imageReqs{VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};

  imageReqs.image = resultImage.image;
  memReqs.pNext   = &dedicatedRegs;

  vkGetImageMemoryRequirements2(m_device, &imageReqs, &memReqs);

  // Build up allocation info
  MemAllocateInfo allocInfo(memReqs.memoryRequirements, memUsage_, true);
  if(dedicatedRegs.requiresDedicatedAllocation)
  {
    allocInfo.setDedicatedImage(resultImage.image);
  }

  // Allocate memory
  resultImage.memHandle = AllocateMemory(allocInfo);
  if(resultImage.memHandle)
  {
    const auto memInfo = m_memAlloc->getMemoryInfo(resultImage.memHandle);
    // Bind memory to image
    NVVK_CHECK(vkBindImageMemory(m_device, resultImage.image, memInfo.memory, memInfo.offset));
  }
  else
  {
    destroy(resultImage);
  }
  return resultImage;
}

Image ResourceAllocator::createImage(const VkCommandBuffer&   cmdBuf,
                                     size_t                   size_,
                                     const void*              data_,
                                     const VkImageCreateInfo& info_,
                                     const VkImageLayout&     layout_)
{
  Image resultImage = createImage(info_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // Copy the data to staging buffer than to image
  if(data_ != nullptr)
  {
    // Copy buffer to image
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.layerCount     = 1;
    subresourceRange.levelCount     = info_.mipLevels;

    // doing these transitions per copy is not efficient, should do in bulk for many images
    nvvk::cmdBarrierImageLayout(cmdBuf, resultImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    VkOffset3D               offset      = {0};
    VkImageSubresourceLayers subresource = {0};
    subresource.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.layerCount               = 1;

    m_staging->cmdToImage(cmdBuf, resultImage.image, offset, info_.extent, subresource, size_, data_);

    // Setting final image layout
    nvvk::cmdBarrierImageLayout(cmdBuf, resultImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout_);
  }
  else
  {
    // Setting final image layout
    nvvk::cmdBarrierImageLayout(cmdBuf, resultImage.image, VK_IMAGE_LAYOUT_UNDEFINED, layout_);
  }

  return resultImage;
}

nvvk::Texture ResourceAllocator::createTexture(const Image&                 image,
                                               const VkImageViewCreateInfo& imageViewCreateInfo,
                                               const VkSamplerCreateInfo&   samplerCreateInfo)
{
  Texture resultTexture            = createTexture(image, imageViewCreateInfo);
  resultTexture.descriptor.sampler = m_samplerPool.acquireSampler(samplerCreateInfo);

  return resultTexture;
}


Texture ResourceAllocator::createTexture(const Image& image, const VkImageViewCreateInfo& imageViewCreateInfo)
{
  Texture resultTexture;
  resultTexture.image                  = image.image;
  resultTexture.memHandle              = image.memHandle;
  resultTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  assert(imageViewCreateInfo.image == image.image);
  NVVK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &resultTexture.descriptor.imageView));

  return resultTexture;
}

Texture ResourceAllocator::createTexture(const VkCommandBuffer&     cmdBuf,
                                         size_t                     size_,
                                         const void*                data_,
                                         const VkImageCreateInfo&   info_,
                                         const VkSamplerCreateInfo& samplerCreateInfo,
                                         const VkImageLayout&       layout_,
                                         bool                       isCube)
{
  Image image = createImage(cmdBuf, size_, data_, info_, layout_);

  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.pNext                           = nullptr;
  viewInfo.image                           = image.image;
  viewInfo.format                          = info_.format;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
  switch(info_.imageType)
  {
    case VK_IMAGE_TYPE_1D:
      viewInfo.viewType = (info_.arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D);
      break;
    case VK_IMAGE_TYPE_2D:
      viewInfo.viewType = isCube ? VK_IMAGE_VIEW_TYPE_CUBE :
                                   (info_.arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
      break;
    case VK_IMAGE_TYPE_3D:
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
      break;
    default:
      assert(0);
  }

  Texture resultTexture                = createTexture(image, viewInfo, samplerCreateInfo);
  resultTexture.descriptor.imageLayout = layout_;
  return resultTexture;
}


SparseImage ResourceAllocator::createSparseImage(VkImageCreateInfo info_, const VkMemoryPropertyFlags memUsage_ /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
{
  SparseImage resultImage;

  std::array<VkImage, SparseImage::s_sparseImageCount> images;
  for(size_t i = 0; i < images.size(); i++)
  {
    if (NVVK_CHECK(vkCreateImage(m_device, &info_, nullptr, &images[i])))
    {
      LOGE("Could not create requested image\n");
      return {};
    }
  }

  std::vector<VkMemoryRequirements> mipTailMemRequirements =
      resultImage.create(m_device, images, info_.mipLevels, info_.arrayLayers, info_.extent);

  std::vector<std::pair<VkDeviceMemory, VkDeviceSize>> mipTailMemAndOffsets;


  for(const auto& memReq : mipTailMemRequirements)
  {
    nvvk::MemAllocateInfo allocInfo(m_device, images[0], memUsage_);
    allocInfo.setMemoryRequirements(memReq);

    nvvk::MemHandle             mipTailAllocationID = AllocateMemory(allocInfo);
    nvvk::MemAllocator::MemInfo memInfo             = m_memAlloc->getMemoryInfo(mipTailAllocationID);

    resultImage.mipTailAllocations.push_back(mipTailAllocationID);
    mipTailMemAndOffsets.push_back({memInfo.memory, memInfo.offset});
  }

  resultImage.bindMipTailMemory(mipTailMemAndOffsets);

  resultImage.memoryProperties = memUsage_;

  return resultImage;
}


void ResourceAllocator::flushSparseImage(SparseImage& sparseImage)
{


  sparseImage.sparseImageMemoryBinds.clear();
  sparseImage.sparseImageMemoryBinds.reserve(sparseImage.allocatedPages.size());
  for(auto it : sparseImage.allocatedPages)
  {
    auto& page = it.second;
    if(!page.hasBoundMemory())
      continue;
    m_memAlloc->freeMemory(page.allocation);
    page.allocation                   = {};
    page.imageMemoryBind.memory       = {};
    page.imageMemoryBind.memoryOffset = {};
    sparseImage.sparseImageMemoryBinds.push_back(page.imageMemoryBind);
  }
  sparseImage.allocatedPages.clear();
  sparseImage.updateSparseBindInfo();
}

// Returns true if the allocation was performed, false if it was already allocated
bool ResourceAllocator::createSparseImagePage(SparseImage& sparseImage, uint32_t pageIndex, uint32_t layer /*= 0u*/)
{
  SparseImage::PageId id{layer, pageIndex};

  auto it = sparseImage.allocatedPages.find(id);
  // If already allocated, nothing to do
  if(it != sparseImage.allocatedPages.end())
  {
    return false;
  }

  SparseImagePage page = sparseImage.createPageInfo(pageIndex, layer);

  VkMemoryRequirements memReqs = sparseImage.memoryReqs;
  memReqs.size                 = page.size;

  nvvk::MemAllocateInfo allocInfo(m_device, sparseImage.images[0], sparseImage.memoryProperties);
  allocInfo.setMemoryRequirements(memReqs);

  nvvk::MemHandle             allocationID = AllocateMemory(static_cast<VkMemoryRequirements>(memReqs));
  nvvk::MemAllocator::MemInfo memInfo      = m_memAlloc->getMemoryInfo(allocationID);
  page.allocation                          = allocationID;
  page.bindDeviceMemory(memInfo.memory, memInfo.offset);

  sparseImage.allocatedPages[id] = page;

  return true;
}


void ResourceAllocator::finalizeStaging(VkFence fence /*= VK_NULL_HANDLE*/)
{
  m_staging->finalizeResources(fence);
}

void ResourceAllocator::releaseStaging()
{
  m_staging->releaseResources();
}

void ResourceAllocator::finalizeAndReleaseStaging(VkFence fence /*= VK_NULL_HANDLE*/)
{
  m_staging->finalizeResources(fence);
  m_staging->releaseResources();
}

nvvk::StagingMemoryManager* ResourceAllocator::getStaging()
{
  return m_staging.get();
}

const nvvk::StagingMemoryManager* ResourceAllocator::getStaging() const
{
  return m_staging.get();
}

void ResourceAllocator::destroy(Buffer& b_)
{
  vkDestroyBuffer(m_device, b_.buffer, nullptr);
  m_memAlloc->freeMemory(b_.memHandle);

  b_ = Buffer();
}

void ResourceAllocator::destroy(Image& i_)
{
  vkDestroyImage(m_device, i_.image, nullptr);

  m_memAlloc->freeMemory(i_.memHandle);
  i_ = Image();
}

void ResourceAllocator::destroy(Texture& t_)
{
  vkDestroyImageView(m_device, t_.descriptor.imageView, nullptr);
  vkDestroyImage(m_device, t_.image, nullptr);
  m_memAlloc->freeMemory(t_.memHandle);

  if(t_.descriptor.sampler)
  {
    m_samplerPool.releaseSampler(t_.descriptor.sampler);
  }

  t_ = Texture();
}


void ResourceAllocator::destroy(nvvk::SparseImage& i_)
{
  flushSparseImage(i_);
  for(auto& mipTailAlloc : i_.mipTailAllocations)
  {
    m_memAlloc->freeMemory(mipTailAlloc);
  }
  i_.mipTailAllocations.clear();
  i_.unbindMipTailMemory();
  for(size_t i = 0; i < nvvk::SparseImage::s_sparseImageCount; i++)
    vkDestroyImage(m_device, i_.images[i], nullptr);
}
bool ResourceAllocator::destroy(nvvk::SparseImage& i_, uint32_t pageIndex, uint32_t layer)
{
  auto it = i_.allocatedPages.find({layer, pageIndex});
  if(it == i_.allocatedPages.end())
  {
    return false;
  }

  SparseImagePage& page = it->second;
  if(!page.hasBoundMemory())
    return false;

  m_memAlloc->freeMemory(page.allocation);
  i_.allocatedPages.erase(it);
  return true;
}

void* ResourceAllocator::map(const Buffer& buffer)
{
  void* pData = m_memAlloc->map(buffer.memHandle);
  return pData;
}

void ResourceAllocator::unmap(const Buffer& buffer)
{
  m_memAlloc->unmap(buffer.memHandle);
}

void* ResourceAllocator::map(const Image& buffer)
{
  void* pData = m_memAlloc->map(buffer.memHandle);
  return pData;
}

void ResourceAllocator::unmap(const Image& image)
{
  m_memAlloc->unmap(image.memHandle);
}

MemHandle ResourceAllocator::AllocateMemory(const MemAllocateInfo& allocateInfo)
{
  return m_memAlloc->allocMemory(allocateInfo);
}

void ResourceAllocator::CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer)
{
  NVVK_CHECK(vkCreateBuffer(m_device, &info_, nullptr, buffer));
}

void ResourceAllocator::CreateImageEx(const VkImageCreateInfo& info_, VkImage* image)
{
  NVVK_CHECK(vkCreateImage(m_device, &info_, nullptr, image));
}

uint32_t ResourceAllocator::getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags& properties)
{
  for(uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
  {
    if(((typeBits & (1 << i)) > 0) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }
  assert(0);
  return ~0u;
}


AccelNV ResourceAllocator::createAcceleration(VkAccelerationStructureCreateInfoNV& accel_)
{
  AccelNV resultAccel;
  // Create the acceleration structure
  NVVK_CHECK(vkCreateAccelerationStructureNV(m_device, &accel_, nullptr, &resultAccel.accel));

  // Find memory requirements
  VkAccelerationStructureMemoryRequirementsInfoNV accelMemInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV};
  accelMemInfo.accelerationStructure = resultAccel.accel;
  VkMemoryRequirements2 memReqs{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &accelMemInfo, &memReqs);

  // Allocate memory
  MemAllocateInfo info(memReqs.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
  resultAccel.memHandle = AllocateMemory(info);
  if(resultAccel.memHandle)
  {
    const auto memInfo = m_memAlloc->getMemoryInfo(resultAccel.memHandle);

    // Bind memory with acceleration structure
    VkBindAccelerationStructureMemoryInfoNV bind{VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV};
    bind.accelerationStructure = resultAccel.accel;
    bind.memory                = memInfo.memory;
    bind.memoryOffset          = memInfo.offset;
    NVVK_CHECK(vkBindAccelerationStructureMemoryNV(m_device, 1, &bind));
  }
  else
  {
    destroy(resultAccel);
  }
  return resultAccel;
}

void ResourceAllocator::destroy(AccelNV& a_)
{
  vkDestroyAccelerationStructureNV(m_device, a_.accel, nullptr);
  m_memAlloc->freeMemory(a_.memHandle);

  a_ = AccelNV();
}

AccelKHR ResourceAllocator::createAcceleration(VkAccelerationStructureCreateInfoKHR& accel_)
{
  AccelKHR resultAccel;
  // Allocating the buffer to hold the acceleration structure
  resultAccel.buffer = createBuffer(accel_.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                                                     | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  // Setting the buffer
  accel_.buffer = resultAccel.buffer.buffer;
  // Create the acceleration structure
  vkCreateAccelerationStructureKHR(m_device, &accel_, nullptr, &resultAccel.accel);

  return resultAccel;
}

void ResourceAllocator::destroy(AccelKHR& a_)
{
  vkDestroyAccelerationStructureKHR(m_device, a_.accel, nullptr);
  destroy(a_.buffer);

  a_ = AccelKHR();
}

VkSampler ResourceAllocator::acquireSampler(const VkSamplerCreateInfo& info)
{
  return m_samplerPool.acquireSampler(info);
}

void ResourceAllocator::releaseSampler(VkSampler sampler)
{
  m_samplerPool.releaseSampler(sampler);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExportResourceAllocator::ExportResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAllocator, VkDeviceSize stagingBlockSize)
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

MemHandle ExportResourceAllocator::AllocateMemory(const MemAllocateInfo& allocateInfo)
{
  MemAllocateInfo exportAllocateInfo(allocateInfo);
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
  m_memAlloc = std::make_unique<DedicatedMemoryAllocator>(device, physicalDevice);
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

ExplicitDeviceMaskResourceAllocator::ExplicitDeviceMaskResourceAllocator(VkDevice         device,
                                                                         VkPhysicalDevice physicalDevice,
                                                                         MemAllocator*    memAlloc,
                                                                         uint32_t         deviceMask)
{
  init(device, physicalDevice, memAlloc, deviceMask);
}

void ExplicitDeviceMaskResourceAllocator::init(VkDevice device, VkPhysicalDevice physicalDevice, MemAllocator* memAlloc, uint32_t deviceMask)
{
  ResourceAllocator::init(device, physicalDevice, memAlloc);
  m_deviceMask = deviceMask;
}

MemHandle ExplicitDeviceMaskResourceAllocator::AllocateMemory(const MemAllocateInfo& allocateInfo)
{
  MemAllocateInfo deviceMaskAllocateInfo(allocateInfo);
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
  m_dma = std::make_unique<DeviceMemoryAllocator>(device, physicalDevice, memBlockSize);
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
  m_memAlloc = std::make_unique<DedicatedMemoryAllocator>(device, physicalDevice);
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

}  // namespace nvvk
