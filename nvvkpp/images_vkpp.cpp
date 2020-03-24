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

#include <nvvkpp/images_vkpp.hpp>


// Return the access flag for an image layout
vk::AccessFlags nvvkpp::image::accessFlagsForLayout(vk::ImageLayout layout)
{
  switch(layout)
  {
    case vk::ImageLayout::ePreinitialized:
      return vk::AccessFlagBits::eHostWrite;
    case vk::ImageLayout::eTransferDstOptimal:
      return vk::AccessFlagBits::eTransferWrite;
    case vk::ImageLayout::eTransferSrcOptimal:
      return vk::AccessFlagBits::eTransferRead;
    case vk::ImageLayout::eColorAttachmentOptimal:
      return vk::AccessFlagBits::eColorAttachmentWrite;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
      return vk::AccessFlagBits::eShaderRead;
    default:
      return vk::AccessFlags();
  }
}

vk::PipelineStageFlags nvvkpp::image::pipelineStageForLayout(vk::ImageLayout layout)
{
  switch(layout)
  {
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eTransferSrcOptimal:
      return vk::PipelineStageFlagBits::eTransfer;
    case vk::ImageLayout::eColorAttachmentOptimal:
      return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      return vk::PipelineStageFlagBits::eEarlyFragmentTests;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
      return vk::PipelineStageFlagBits::eFragmentShader;
    case vk::ImageLayout::ePreinitialized:
      return vk::PipelineStageFlagBits::eHost;
    case vk::ImageLayout::eUndefined:
      return vk::PipelineStageFlagBits::eTopOfPipe;
    default:
      return vk::PipelineStageFlagBits::eBottomOfPipe;
  }
}

void nvvkpp::image::setImageLayout(const vk::CommandBuffer&         cmdbuffer,
                                   const vk::Image&                 image,
                                   const vk::ImageLayout&           oldImageLayout,
                                   const vk::ImageLayout&           newImageLayout,
                                   const vk::ImageSubresourceRange& subresourceRange)
{
  // Create an image barrier to change the layout
  vk::ImageMemoryBarrier imageMemoryBarrier;
  imageMemoryBarrier.oldLayout         = oldImageLayout;
  imageMemoryBarrier.newLayout         = newImageLayout;
  imageMemoryBarrier.image             = image;
  imageMemoryBarrier.subresourceRange  = subresourceRange;
  imageMemoryBarrier.srcAccessMask     = accessFlagsForLayout(oldImageLayout);
  imageMemoryBarrier.dstAccessMask     = accessFlagsForLayout(newImageLayout);
  vk::PipelineStageFlags srcStageMask  = pipelineStageForLayout(oldImageLayout);
  vk::PipelineStageFlags destStageMask = pipelineStageForLayout(newImageLayout);
  cmdbuffer.pipelineBarrier(srcStageMask, destStageMask, vk::DependencyFlags(), nullptr, nullptr, imageMemoryBarrier);
}

void nvvkpp::image::setImageLayout(const vk::CommandBuffer&    cmdbuffer,
                                   const vk::Image&            image,
                                   const vk::ImageAspectFlags& aspectMask,
                                   const vk::ImageLayout&      oldImageLayout,
                                   const vk::ImageLayout&      newImageLayout)
{
  vk::ImageSubresourceRange subresourceRange;
  subresourceRange.aspectMask = aspectMask;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = 1;
  setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange);
}

vk::ImageCreateInfo nvvkpp::image::create2DInfo(const vk::Extent2D&        size,
                                                const vk::Format&          format,
                                                const vk::ImageUsageFlags& usage,
                                                const bool&                mipmaps)
{
  vk::ImageCreateInfo icInfo;
  icInfo.imageType   = vk::ImageType::e2D;
  icInfo.format      = format;
  icInfo.mipLevels   = mipmaps ? nvvkpp::image::mipLevels(size) : 1;
  icInfo.arrayLayers = 1;
  icInfo.extent      = vk::Extent3D(size.width, size.height, 1);
  icInfo.usage       = usage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
  return icInfo;
}

vk::DescriptorImageInfo nvvkpp::image::create2DDescriptor(const vk::Device&            device,
                                                          const vk::Image&             image,
                                                          const vk::SamplerCreateInfo& samplerCreateInfo,
                                                          const vk::Format&            format,
                                                          const vk::ImageLayout&       layout)
{
  vk::DescriptorImageInfo texDesc;
  vk::ImageViewCreateInfo viewCreateInfo{{},     image, vk::ImageViewType::e2D,
                                         format, {},    {vk::ImageAspectFlagBits::eColor, 0, ~0u, 0, 1}};

  texDesc.setSampler(device.createSampler(samplerCreateInfo));
  texDesc.setImageView(device.createImageView(viewCreateInfo));
  texDesc.setImageLayout(layout);

  return texDesc;
}

vk::ImageCreateInfo nvvkpp::image::create3DInfo(const vk::Extent3D&        size,
  const vk::Format&          format,
  const vk::ImageUsageFlags& usage,
  const bool&                mipmaps)
{
  vk::ImageCreateInfo icInfo;
  icInfo.imageType = vk::ImageType::e3D;
  icInfo.format = format;
  icInfo.mipLevels = mipmaps ? nvvkpp::image::mipLevels(size) : 1;
  icInfo.arrayLayers = 1;
  icInfo.extent = vk::Extent3D(size.width, size.height, size.width);
  icInfo.usage = usage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
  return icInfo;
}

vk::DescriptorImageInfo nvvkpp::image::create3DDescriptor(const vk::Device&            device,
  const vk::Image&             image,
  const vk::SamplerCreateInfo& samplerCreateInfo,
  const vk::Format&            format,
  const vk::ImageLayout&       layout)
{
  vk::DescriptorImageInfo texDesc;
  vk::ImageViewCreateInfo viewCreateInfo{ {},     image, vk::ImageViewType::e3D,
                                         format, {},    {vk::ImageAspectFlagBits::eColor, 0, ~0u, 0, 1} };

  texDesc.setSampler(device.createSampler(samplerCreateInfo));
  texDesc.setImageView(device.createImageView(viewCreateInfo));
  texDesc.setImageLayout(layout);

  return texDesc;
}

vk::ImageCreateInfo nvvkpp::image::createCubeInfo(const vk::Extent2D&        size,
                                                  const vk::Format&          format,
                                                  const vk::ImageUsageFlags& usage,
                                                  const bool&                mipmaps)
{
  vk::ImageCreateInfo icInfo;
  icInfo.imageType   = vk::ImageType::e2D;
  icInfo.format      = format;
  icInfo.mipLevels   = mipmaps ? nvvkpp::image::mipLevels(size) : 1;
  icInfo.arrayLayers = 6;
  icInfo.extent      = vk::Extent3D(size.width, size.height, 1);
  icInfo.usage       = usage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
  icInfo.flags       = vk::ImageCreateFlagBits::eCubeCompatible;

  return icInfo;
}

vk::DescriptorImageInfo nvvkpp::image::createCubeDescriptor(const vk::Device&            device,
                                                            const vk::Image&             image,
                                                            const vk::SamplerCreateInfo& samplerCreateInfo,
                                                            const vk::Format&            format,
                                                            const vk::ImageLayout&       layout)
{
  
  vk::ImageSubresourceRange range;
  range.setAspectMask(vk::ImageAspectFlagBits::eColor);
  range.setBaseArrayLayer(0);
  range.setBaseMipLevel(0);
  range.setLayerCount(6);
  range.setLevelCount(~0u);
  vk::ImageViewCreateInfo viewCreateInfo;
  viewCreateInfo.setFormat(format);
  viewCreateInfo.setImage(image);
  viewCreateInfo.setSubresourceRange(range);
  viewCreateInfo.setViewType(vk::ImageViewType::eCube);
  vk::DescriptorImageInfo texDesc;
  texDesc.setSampler(device.createSampler(samplerCreateInfo));
  texDesc.setImageView(device.createImageView(viewCreateInfo));
  texDesc.setImageLayout(layout);
  return texDesc;
}

// This mipmap generation relies on blitting
// A more sophisticated version could be done with computer shader
// We will publish how to in the future

void nvvkpp::image::generateMipmaps(const vk::CommandBuffer& cmdBuf,
                                    const vk::Image&         image,
                                    const vk::Format&        imageFormat,
                                    const vk::Extent2D&      size,
                                    const uint32_t&          mipLevels)
{
  // Transfer the top level image to a layout 'eTransferSrcOptimal` and its access to 'eTransferRead'
  vk::ImageMemoryBarrier barrier;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;
  barrier.image                           = image;
  barrier.oldLayout                       = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.newLayout                       = vk::ImageLayout::eTransferSrcOptimal;
  barrier.srcAccessMask                   = vk::AccessFlags();
  barrier.dstAccessMask                   = vk::AccessFlagBits::eTransferRead;
  cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                         vk::DependencyFlags(), nullptr, nullptr, barrier);


  int32_t mipWidth  = size.width;
  int32_t mipHeight = size.height;

  for(uint32_t i = 1; i < mipLevels; i++)
  {

    vk::ImageBlit blit;
    blit.srcOffsets[0]                 = vk::Offset3D(0, 0, 0);
    blit.srcOffsets[1]                 = vk::Offset3D(mipWidth, mipHeight, 1);
    blit.srcSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = vk::Offset3D(0, 0, 0);
    blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
    blit.dstSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    cmdBuf.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, {blit},
                     vk::Filter::eLinear);


    // Next
    if(i + 1 < mipLevels)
    {
      // Transition the current miplevel into a eTransferSrcOptimal layout, to be used as the source for the next one.
      barrier.subresourceRange.baseMipLevel = i;
      barrier.oldLayout                     = vk::ImageLayout::eTransferDstOptimal;
      barrier.newLayout                     = vk::ImageLayout::eTransferSrcOptimal;
      barrier.srcAccessMask                 = vk::AccessFlags();  //vk::AccessFlagBits::eTransferWrite;
      barrier.dstAccessMask                 = vk::AccessFlagBits::eTransferRead;
      cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                             vk::DependencyFlags(), nullptr, nullptr, barrier);
    }


    if(mipWidth > 1)
      mipWidth /= 2;
    if(mipHeight > 1)
      mipHeight /= 2;
  }

  //Transition all miplevels into a eShaderReadOnlyOptimal layout.
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount   = mipLevels;
  barrier.oldLayout                     = vk::ImageLayout::eUndefined;
  barrier.newLayout                     = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcAccessMask                 = vk::AccessFlags();
  barrier.dstAccessMask                 = vk::AccessFlagBits::eShaderRead;
  cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                         vk::DependencyFlags(), nullptr, nullptr, barrier);
}
