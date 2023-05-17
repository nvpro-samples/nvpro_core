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


#pragma once

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cmath>

namespace nvvk {
//////////////////////////////////////////////////////////////////////////
/**
    # functions in nvvk

    - makeImageMemoryBarrier : returns VkImageMemoryBarrier for an image based on provided layouts and access flags.
    - mipLevels : return number of mips for 2d/3d extent

    - accessFlagsForImageLayout : helps resource transtions
    - pipelineStageForLayout : helps resource transitions
    - cmdBarrierImageLayout : inserts barrier for image transition

    - cmdGenerateMipmaps : basic mipmap creation for images (meant for one-shot operations)

    - makeImage2DCreateInfo : aids 2d image creation
    - makeImage3DCreateInfo : aids 3d descriptor set updating
    - makeImageCubeCreateInfo : aids cube descriptor set updating
    - makeImageViewCreateInfo : aids common image view creation, derives info from VkImageCreateInfo
    - makeImage2DViewCreateInfo : aids 2d image view creation
  */

VkImageMemoryBarrier makeImageMemoryBarrier(VkImage            image,
                                            VkAccessFlags      srcAccess,
                                            VkAccessFlags      dstAccess,
                                            VkImageLayout      oldLayout,
                                            VkImageLayout      newLayout,
                                            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);


//--------------------------------------------------------------------------------------------------
inline uint32_t mipLevels(VkExtent2D extent)
{
  return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

inline uint32_t mipLevels(VkExtent3D extent)
{
  return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

//--------------------------------------------------------------------------------------------------
// Transition Pipeline Layout tools

VkAccessFlags        accessFlagsForImageLayout(VkImageLayout layout);
VkPipelineStageFlags pipelineStageForLayout(VkImageLayout layout);

void cmdBarrierImageLayout(VkCommandBuffer                cmdbuffer,
                           VkImage                        image,
                           VkImageLayout                  oldImageLayout,
                           VkImageLayout                  newImageLayout,
                           const VkImageSubresourceRange& subresourceRange);

void cmdBarrierImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageAspectFlags aspectMask);

inline void cmdBarrierImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
{
  cmdBarrierImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, VK_IMAGE_ASPECT_COLOR_BIT);
}


VkImageCreateInfo makeImage3DCreateInfo(const VkExtent3D& size,
                                        VkFormat          format  = VK_FORMAT_R8G8B8A8_UNORM,
                                        VkImageUsageFlags usage   = VK_IMAGE_USAGE_SAMPLED_BIT,
                                        bool              mipmaps = false);


VkImageCreateInfo makeImage2DCreateInfo(const VkExtent2D& size,
                                        VkFormat          format  = VK_FORMAT_R8G8B8A8_UNORM,
                                        VkImageUsageFlags usage   = VK_IMAGE_USAGE_SAMPLED_BIT,
                                        bool              mipmaps = false);

VkImageCreateInfo makeImageCubeCreateInfo(const VkExtent2D& size,
                                          VkFormat          format  = VK_FORMAT_R8G8B8A8_UNORM,
                                          VkImageUsageFlags usage   = VK_IMAGE_USAGE_SAMPLED_BIT,
                                          bool              mipmaps = false);

// derives format and view type from imageInfo, special case for IMAGE_2D to treat as cube
// view enables all mips and layers
VkImageViewCreateInfo makeImageViewCreateInfo(VkImage image, const VkImageCreateInfo& imageInfo, bool isCube = false);


VkImageViewCreateInfo makeImage2DViewCreateInfo(VkImage            image,
                                                VkFormat           format         = VK_FORMAT_R8G8B8A8_UNORM,
                                                VkImageAspectFlags aspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT,
                                                uint32_t           levels         = VK_REMAINING_MIP_LEVELS,
                                                const void*        pNextImageView = nullptr);

void cmdGenerateMipmaps(VkCommandBuffer   cmdBuf,
                        VkImage           image,
                        VkFormat          imageFormat,
                        const VkExtent2D& size,
                        uint32_t          levelCount,
                        uint32_t          layerCount    = 1,
                        VkImageLayout     currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

}  // namespace nvvk
