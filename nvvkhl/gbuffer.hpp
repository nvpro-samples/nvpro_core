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

#pragma once
#include "vulkan/vulkan_core.h"
#include "nvvk/resourceallocator_vk.hpp"

namespace nvvkhl {
class GBuffer
{
public:
  GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc);
  GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc, const VkExtent2D& size, VkFormat color, VkFormat depth);
  GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc, const VkExtent2D& size, std::vector<VkFormat> color, VkFormat depth);
  ~GBuffer();

  void create(const VkExtent2D& size, std::vector<VkFormat> color, VkFormat depth);
  void destroy();

  VkDescriptorSet       getDescriptorSet(uint32_t i = 0) const { return m_descriptorSet[i]; }
  VkExtent2D            getSize() const { return m_imageSize; }
  VkImage               getColorImage(uint32_t i = 0) const { return m_res.gBufferColor[i].image; }
  VkImage               getDepthImage() const { return m_res.gBufferDepth.image; }
  VkImageView           getColorImageView(uint32_t i = 0) const { return m_res.descriptor[i].imageView; }
  VkDescriptorImageInfo getDescriptorImageInfo(uint32_t i = 0) const { return m_res.descriptor[i]; }
  VkImageView           getDepthImageView() const { return m_res.depthView; }
  VkFormat              getColorFormat(uint32_t i = 0) const { return m_colorFormat[i]; }
  VkFormat              getDepthFormat() const { return m_depthFormat; }
  float getAspectRatio() { return static_cast<float>(m_imageSize.width) / static_cast<float>(m_imageSize.height); }

  // Create a buffer from the VkImage, useful for saving to disk
  nvvk::Buffer createImageToBuffer(VkCommandBuffer cmd, uint32_t i = 0) const;

private:
  struct Resources
  {
    std::vector<nvvk::Image>           gBufferColor;                // All color image to render into
    nvvk::Image                        gBufferDepth;                // Depth buffer
    VkImageView                        depthView = VK_NULL_HANDLE;  // Image view of the depth buffer
    std::vector<VkDescriptorImageInfo> descriptor;                  // Holds the sampler and image view
    VkSampler                          linearSampler = VK_NULL_HANDLE;
  };

  Resources                    m_res;
  VkExtent2D                   m_imageSize{0U, 0U};                           // Current image size
  std::vector<VkFormat>        m_colorFormat;                                 // Color format of the image
  VkFormat                     m_depthFormat{VK_FORMAT_X8_D24_UNORM_PACK32};  // Depth format of the depth buffer
  std::vector<VkDescriptorSet> m_descriptorSet;                               // For displaying the image with ImGui

  VkDevice                 m_device{VK_NULL_HANDLE};
  nvvk::ResourceAllocator* m_alloc;
};

}  // namespace nvvkhl
