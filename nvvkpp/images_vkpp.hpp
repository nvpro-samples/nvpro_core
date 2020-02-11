/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#pragma once
#include <cmath>
#include <vulkan/vulkan.hpp>

namespace nvvkpp {

//--------------------------------------------------------------------------------------------------
/**
Various Image Utilities
- Transition Pipeline Layout tools
- 2D Texture helper creation
- Cubic Texture helper creation
*/


namespace image {

/**
**mipLevels**: Returns the number of mipmaps an image can have
*/
inline uint32_t mipLevels(vk::Extent2D extent)
{
  return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

inline uint32_t mipLevels(vk::Extent3D extent)
{
  return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

//--------------------------------------------------------------------------------------------------
/**
**setImageLayout**, **accessFlagsForLayout**, **pipelineStageForLayout**: Transition Pipeline Layout tools
*/
vk::AccessFlags        accessFlagsForLayout(vk::ImageLayout layout);
vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout);

void setImageLayout(const vk::CommandBuffer&         cmdbuffer,
                    const vk::Image&                 image,
                    const vk::ImageLayout&           oldImageLayout,
                    const vk::ImageLayout&           newImageLayout,
                    const vk::ImageSubresourceRange& subresourceRange);

void setImageLayout(const vk::CommandBuffer&    cmdbuffer,
                    const vk::Image&            image,
                    const vk::ImageAspectFlags& aspectMask,
                    const vk::ImageLayout&      oldImageLayout,
                    const vk::ImageLayout&      newImageLayout);

inline void setImageLayout(const vk::CommandBuffer& cmdbuffer,
                           const vk::Image&         image,
                           const vk::ImageLayout&   oldImageLayout,
                           const vk::ImageLayout&   newImageLayout)
{
  setImageLayout(cmdbuffer, image, vk::ImageAspectFlagBits::eColor, oldImageLayout, newImageLayout);
}

//--------------------------------------------------------------------------------------------------
/**
**create2DInfo**: creates a vk::ImageCreateInfo

**create2DDescriptor**: creates vk::create2DDescriptor

**generateMipmaps**: generate all mipmaps for a vk::Image
*/
vk::ImageCreateInfo create3DInfo(const vk::Extent3D&        size,
  const vk::Format&          format = vk::Format::eR8G8B8A8Unorm,
  const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eSampled,
  const bool&                mipmaps = false);

vk::DescriptorImageInfo create3DDescriptor(const vk::Device&            device,
  const vk::Image&             image,
  const vk::SamplerCreateInfo& samplerCreateInfo = vk::SamplerCreateInfo(),
  const vk::Format&            format = vk::Format::eR8G8B8A8Unorm,
  const vk::ImageLayout& layout = vk::ImageLayout::eShaderReadOnlyOptimal);

//--------------------------------------------------------------------------------------------------
/**
**create2DInfo**: creates a vk::ImageCreateInfo

**create2DDescriptor**: creates vk::create2DDescriptor

**generateMipmaps**: generate all mipmaps for a vk::Image 
*/
vk::ImageCreateInfo create2DInfo(const vk::Extent2D&        size,
                                 const vk::Format&          format  = vk::Format::eR8G8B8A8Unorm,
                                 const vk::ImageUsageFlags& usage   = vk::ImageUsageFlagBits::eSampled,
                                 const bool&                mipmaps = false);

vk::DescriptorImageInfo create2DDescriptor(const vk::Device&            device,
                                           const vk::Image&             image,
                                           const vk::SamplerCreateInfo& samplerCreateInfo = vk::SamplerCreateInfo(),
                                           const vk::Format&            format            = vk::Format::eR8G8B8A8Unorm,
                                           const vk::ImageLayout& layout = vk::ImageLayout::eShaderReadOnlyOptimal);

void generateMipmaps(const vk::CommandBuffer& cmdBuf,
                     const vk::Image&         image,
                     const vk::Format&        imageFormat,
                     const vk::Extent2D&      size,
                     const uint32_t&          mipLevels);
//--------------------------------------------------------------------------------------------------
// 2D Texture helper creation
//
vk::ImageCreateInfo createCubeInfo(const vk::Extent2D&        size,
                                   const vk::Format&          format  = vk::Format::eR8G8B8A8Unorm,
                                   const vk::ImageUsageFlags& usage   = vk::ImageUsageFlagBits::eSampled,
                                   const bool&                mipmaps = false);

vk::DescriptorImageInfo createCubeDescriptor(const vk::Device&            device,
                                             const vk::Image&             image,
                                             const vk::SamplerCreateInfo& samplerCreateInfo,
                                             const vk::Format&            format = vk::Format::eR8G8B8A8Unorm,
                                             const vk::ImageLayout& layout = vk::ImageLayout::eShaderReadOnlyOptimal);


}  // namespace image
}  // namespace nvvkpp
