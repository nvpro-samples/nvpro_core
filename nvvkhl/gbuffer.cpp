/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2023 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gbuffer.hpp"

#include <utility>
#include "application.hpp"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "nvvk/images_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/commands_vk.hpp"

nvvkhl::GBuffer::GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc)
    : m_device(device)
    , m_alloc(alloc)
{
}

nvvkhl::GBuffer::GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc, const VkExtent2D& size, VkFormat color, VkFormat depth)
    : m_device(device)
    , m_alloc(alloc)
{
  create(size, {color}, depth);
}

nvvkhl::GBuffer::GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc, const VkExtent2D& size, std::vector<VkFormat> color, VkFormat depth)
    : m_device(device)
    , m_alloc(alloc)
{
  create(size, color, depth);
}

nvvkhl::GBuffer::~GBuffer()
{
  destroy();
}

void nvvkhl::GBuffer::create(const VkExtent2D& size, std::vector<VkFormat> color, VkFormat depth)
{
  assert(m_colorFormat.empty());  // The buffer must be cleared before creating a new one

  m_imageSize   = size;
  m_colorFormat = std::move(color);
  m_depthFormat = depth;

  nvvk::DebugUtil dutil(m_device);

  VkImageLayout layout{VK_IMAGE_LAYOUT_GENERAL};


  auto num_color = static_cast<uint32_t>(m_colorFormat.size());

  m_res.gBufferColor.resize(num_color);
  m_res.descriptor.resize(num_color);

  for(uint32_t c = 0; c < num_color; c++)
  {
    {  // Color image
      VkImageUsageFlags flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
      VkImageCreateInfo info = nvvk::makeImage2DCreateInfo(m_imageSize, m_colorFormat[c], flags);
      m_res.gBufferColor[c]  = m_alloc->createImage(info);
      dutil.setObjectName(m_res.gBufferColor[c].image, "G-Color" + std::to_string(c));
    }
    {  // Image color view
      VkImageViewCreateInfo info = nvvk::makeImage2DViewCreateInfo(m_res.gBufferColor[c].image, m_colorFormat[c]);
      vkCreateImageView(m_device, &info, nullptr, &m_res.descriptor[c].imageView);
      dutil.setObjectName(m_res.descriptor[c].imageView, "G-Color" + std::to_string(c));
    }

    if(m_res.descriptor[c].sampler == VK_NULL_HANDLE)
    {  // Image sampler: nearest sampling by default
      VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
      m_res.descriptor[c].sampler = m_alloc->acquireSampler(info);
      dutil.setObjectName(m_res.descriptor[c].sampler, "G-Sampler");
    }
  }

  {  // Depth buffer
    VkImageCreateInfo info = nvvk::makeImage2DCreateInfo(m_imageSize, m_depthFormat,
                                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    m_res.gBufferDepth     = m_alloc->createImage(info);
    dutil.setObjectName(m_res.gBufferDepth.image, "G-Depth");
  }

  {  // Image depth view
    VkImageViewCreateInfo info = nvvk::makeImage2DViewCreateInfo(m_res.gBufferDepth.image, m_depthFormat);
    info.subresourceRange      = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    vkCreateImageView(m_device, &info, nullptr, &m_res.depthView);
    dutil.setObjectName(m_res.depthView, "G-Depth");
  }


  {  // Change color image layout
    nvvk::CommandPool cpool(m_device, 0);
    VkCommandBuffer   cmd = cpool.createCommandBuffer();
    for(uint32_t c = 0; c < num_color; c++)
    {
      nvvk::cmdBarrierImageLayout(cmd, m_res.gBufferColor[c].image, VK_IMAGE_LAYOUT_UNDEFINED, layout);
      m_res.descriptor[c].imageLayout = layout;

      // Clear to avoid garbage data
      VkClearColorValue       clear_value = {{0.F, 0.F, 0.F, 0.F}};
      VkImageSubresourceRange range       = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCmdClearColorImage(cmd, m_res.gBufferColor[c].image, layout, &clear_value, 1, &range);
    }
    cpool.submitAndWait(cmd);
  }

  // Descriptor Set for ImGUI
  if((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
  {
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    info.minFilter      = VK_FILTER_LINEAR;
    info.magFilter      = VK_FILTER_LINEAR;
    m_res.linearSampler = m_alloc->acquireSampler(info);
    for(const VkDescriptorImageInfo& desc : m_res.descriptor)
    {
      m_descriptorSet.push_back(ImGui_ImplVulkan_AddTexture(m_res.linearSampler, desc.imageView, layout));
    }
  }
}

//-------------------------------------------
// Destroying all allocated resources
//
void nvvkhl::GBuffer::destroy()
{
  if((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
  {
    for(VkDescriptorSet set : m_descriptorSet)
    {
      ImGui_ImplVulkan_RemoveTexture(set);
    }

    if(m_res.linearSampler)
      m_alloc->releaseSampler(m_res.linearSampler);
  }

  for(nvvk::Image bc : m_res.gBufferColor)
  {
    m_alloc->destroy(bc);
  }

  m_alloc->destroy(m_res.gBufferDepth);

  vkDestroyImageView(m_device, m_res.depthView, nullptr);

  for(const VkDescriptorImageInfo& desc : m_res.descriptor)
  {
    vkDestroyImageView(m_device, desc.imageView, nullptr);
    m_alloc->releaseSampler(desc.sampler);
  }

  // Reset everything to zero
  m_res       = {};
  m_imageSize = {};
  m_colorFormat.clear();
  m_descriptorSet.clear();
}

//--------------------------------------------------------------------------------------------------------------
// Creating a buffer from one of the color image.
// This can be used to save image to disk:
//
// Note: it is the responsibility to the function who call this, to destroy the buffer.
nvvk::Buffer nvvkhl::GBuffer::createImageToBuffer(VkCommandBuffer cmd, uint32_t i /*= 0*/) const
{
  // Source image
  VkImage    src_image = getColorImage(i);
  VkExtent2D img_size  = getSize();

  VkFormat format        = getColorFormat(i);
  uint32_t bytesPerPixel = 0;
  if(format >= VK_FORMAT_R8G8B8A8_UNORM && format <= VK_FORMAT_B8G8R8A8_SRGB)
    bytesPerPixel = 4 * sizeof(uint8_t);
  else if(format >= VK_FORMAT_R16G16B16A16_UNORM && format <= VK_FORMAT_R16G16B16A16_SFLOAT)
    bytesPerPixel = 4 * sizeof(uint16_t);
  else if(format >= VK_FORMAT_R32G32B32A32_UINT && format <= VK_FORMAT_R32G32B32A32_SFLOAT)
    bytesPerPixel = 4 * sizeof(uint32_t);
  assert(bytesPerPixel != 0);  // Format unsupported

  // Destination buffer
  size_t       buf_size   = static_cast<size_t>(bytesPerPixel) * img_size.width * img_size.height;
  nvvk::Buffer dst_buffer = m_alloc->createBuffer(buf_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  // Region to copy from the image (all)
  VkBufferImageCopy region           = {};
  region.bufferRowLength             = img_size.width;
  region.bufferImageHeight           = img_size.height;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent.width           = img_size.width;
  region.imageExtent.height          = img_size.height;
  region.imageExtent.depth           = 1;

  // Copy the image to buffer
  nvvk::cmdBarrierImageLayout(cmd, src_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  vkCmdCopyImageToBuffer(cmd, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_buffer.buffer, 1, &region);
  nvvk::cmdBarrierImageLayout(cmd, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

  // Barrier to make sure work is done
  VkMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  memBarrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &memBarrier, 0,
                       nullptr, 0, nullptr);

  return dst_buffer;
}
