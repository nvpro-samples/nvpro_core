/* Copyright (c) 2014-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "images_vk.hpp"
#include "commands_vk.hpp"
#include <assert.h>

namespace nvvk {

VkImage createImage2D(VkDevice              device,
                      uint32_t              width,
                      uint32_t              height,
                      VkFormat              format,
                      VkImageUsageFlags     usage /*= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT*/,
                      uint32_t              levels /*= 1*/,
                      VkSampleCountFlagBits samples /*= VK_SAMPLE_COUNT_1_BIT*/,
                      VkImageTiling         tiling /*= VK_IMAGE_TILING_OPTIMAL*/,
                      const void*           pNextImage /*= nullptr*/)
{
  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

  imageInfo.pNext         = pNextImage;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = levels;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.samples       = samples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  VkImage image = VK_NULL_HANDLE;
  if(vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
  {
    assert(0 && "failed to create texture image");
  }

  return image;
}

VkImageView createImage2DView(VkDevice           device,
                              VkImage            image,
                              VkFormat           format,
                              VkImageAspectFlags aspectFlags /*= VK_IMAGE_ASPECT_COLOR_BIT*/,
                              uint32_t           levels /*= 1*/,
                              const void*        pNextImageView)
{
  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.pNext                           = pNextImageView;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = levels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  VkImageView imageView = VK_NULL_HANDLE;
  if(vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    assert(0 && "failed to create texture image view!");
  }

  return imageView;
}

//--------------------------------------------------------------------------------------------------
// To copy a source image into a destination image, potentially performing
// format conversion, arbitrary scaling, and filtering
//
void cmdBlitImage(VkCommandBuffer cmdBuf, VkImage imageFrom, std::array<int, 2> sizeFrom, VkImage imageTo, std::array<int, 2> sizeTo, VkFilter filter)
{
  VkImageBlit blit               = {};
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.layerCount = 1;
  blit.srcSubresource.mipLevel   = 0;
  blit.srcOffsets[1].x           = sizeFrom[0];
  blit.srcOffsets[1].y           = sizeFrom[1];
  blit.srcOffsets[1].z           = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.layerCount = 1;
  blit.dstSubresource.mipLevel   = 0;
  blit.dstOffsets[1].x           = sizeTo[0];
  blit.dstOffsets[1].y           = sizeTo[1];
  blit.dstOffsets[1].z           = 1;
  vkCmdBlitImage(cmdBuf, imageFrom, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageTo, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1, &blit, filter);
}

void cmdTransitionImage(VkCommandBuffer commandBuffer,
                        VkImage         image,
                        VkFormat        format,
                        VkImageLayout   oldLayout,
                        VkImageLayout   newLayout,
                        uint32_t        baseMipLevel,
                        uint32_t        levelCount,
                        uint32_t        baseArrayLayer,
                        uint32_t        layerCount,
                        const void*     pNextBarrier)
{
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.pNext                           = pNextBarrier;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.image                           = image;
  barrier.subresourceRange.baseMipLevel   = baseMipLevel;
  barrier.subresourceRange.levelCount     = levelCount;
  barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
  barrier.subresourceRange.layerCount     = layerCount;

  VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  switch(oldLayout)
  {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      barrier.srcAccessMask = 0;
      break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
      srcStageMask          = VK_PIPELINE_STAGE_HOST_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      srcStageMask          = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      srcStageMask          = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      srcStageMask          = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      break;
    default:
      assert(0 && "unsupported layout transition: oldLayout unknown!");
      break;
  }

  switch(newLayout)
  {
    case VK_IMAGE_LAYOUT_GENERAL:
      barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dstStageMask          = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      dstStageMask          = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      dstStageMask          = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dstStageMask          = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      barrier.dstAccessMask = 0;  // this is ok; see spec
      break;
    default:
      assert(0 && "unsupported layout transition: newLayout unknown!");
      break;
  }

  if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if(format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

//////////////////////////////////////////////////////////////////////////

void DedicatedImage::init(VkDevice                     device,
                          VkPhysicalDevice             physical,
                          const VkImageCreateInfo&     imageInfo,
                          VkMemoryPropertyFlags        memoryPropertyFlags,
                          const void*                  pNextMemory /*= nullptr*/)
{

  m_device    = device;

  if(vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
  {
    assert(0 && "image create failed");
  }

  VkMemoryRequirements2          memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements  dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkImageMemoryRequirementsInfo2 imageReqs     = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};

  imageReqs.image = m_image;
  memReqs.pNext   = &dedicatedRegs;
  vkGetImageMemoryRequirements2(device, &imageReqs, &memReqs);

  VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV};
  dedicatedInfo.image                         = m_image;
  dedicatedInfo.pNext                         = pNextMemory;

  VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.pNext          = &dedicatedInfo;
  allocInfo.allocationSize = memReqs.memoryRequirements.size;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);

  // Find an available memory type that satisfies the requested properties.
  for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
  {
    if((memReqs.memoryRequirements.memoryTypeBits & (1 << memoryTypeIndex))
       && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
    {
      allocInfo.memoryTypeIndex = memoryTypeIndex;
      break;
    }
  }
  assert(allocInfo.memoryTypeIndex != ~0);

  if(vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
  {
    assert(0 && "failed to allocate image memory!");
  }

  vkBindImageMemory(device, m_image, m_memory, 0);
}

void DedicatedImage::initWithView(VkDevice              device,
                                  VkPhysicalDevice      physical,
                                  uint32_t              width,
                                  uint32_t              height,
                                  uint32_t              layers,
                                  VkFormat              format,
                                  VkImageUsageFlags     usage /*= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT*/,
                                  VkImageTiling         tiling /*= VK_IMAGE_TILING_OPTIMAL*/,
                                  VkMemoryPropertyFlags memoryPropertyFlags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/,
                                  VkSampleCountFlagBits samples /*= VK_SAMPLE_COUNT_1_BIT*/,
                                  VkImageAspectFlags    aspect /*= VK_IMAGE_ASPECT_COLOR_BIT*/,
                                  const void*           pNextImage /*= nullptr*/,
                                  const void*           pNextMemory /*= nullptr*/,
                                  const void*           pNextImageView /*= nullptr*/)
{
  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.pNext         = pNextImage;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = layers;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.samples       = samples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  init(device, physical, imageInfo, memoryPropertyFlags, pNextMemory);
  initView(imageInfo, aspect, layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D, pNextImageView);
}

void DedicatedImage::initView(const VkImageCreateInfo& imageInfo, VkImageAspectFlags aspect, VkImageViewType viewType, const void* pNextImageView /*= nullptr*/)
{
  VkImageViewCreateInfo createInfo           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  createInfo.pNext                           = pNextImageView;
  createInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
  createInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
  createInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
  createInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
  createInfo.subresourceRange.aspectMask     = aspect;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.baseMipLevel   = 0;
  createInfo.subresourceRange.layerCount     = imageInfo.arrayLayers;
  createInfo.subresourceRange.levelCount     = imageInfo.mipLevels;
  createInfo.format                          = imageInfo.format;
  createInfo.viewType                        = viewType;
  createInfo.image                           = m_image;

  VkResult result = vkCreateImageView(m_device, &createInfo, nullptr, &m_imageView);
  assert(result == VK_SUCCESS);
}

void DedicatedImage::deinit()
{
  if(m_image != nullptr)
    vkDestroyImage(m_device, m_image, nullptr);
  if(m_imageView != nullptr)
    vkDestroyImageView(m_device, m_imageView, nullptr);
  if(m_memory != nullptr)
    vkFreeMemory(m_device, m_memory, nullptr);
  *this = {};
}

void DedicatedImage::cmdInitialTransition(VkCommandBuffer cmd, VkImageLayout layout, VkAccessFlags access)
{
  VkPipelineStageFlags srcPipe = nvvk::makeAccessMaskPipelineStageFlags(0);
  VkPipelineStageFlags dstPipe = nvvk::makeAccessMaskPipelineStageFlags(access);

  VkImageMemoryBarrier memBarrier = makeImageMemoryBarrier(m_image, 0, access, VK_IMAGE_LAYOUT_UNDEFINED, layout);

  vkCmdPipelineBarrier(cmd, srcPipe, dstPipe, VK_FALSE, 0, NULL, 0, NULL, 1, &memBarrier);
}

}  // namespace nvvk
