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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gbuffer.hpp"

#include <utility>
#include "application.hpp"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "nvvk/images_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/commands_vk.hpp"

nvvkhl::GBuffer::GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc, const VkExtent2D& size, VkFormat color, VkFormat depth)
    : m_device(device)
    , m_imageSize(size)
    , m_alloc(alloc)
    , m_colorFormat({color})  // Only one color buffer
    , m_depthFormat(depth)
{
  create();
}

nvvkhl::GBuffer::GBuffer(VkDevice device, nvvk::ResourceAllocator* alloc, const VkExtent2D& size, std::vector<VkFormat> color, VkFormat depth)
    : m_device(device)
    , m_imageSize(size)
    , m_alloc(alloc)
    , m_colorFormat(std::move(color))
    , m_depthFormat(depth)
{
  create();
}

void nvvkhl::GBuffer::create()
{
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
    {  // Image sampler
      VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
      m_res.descriptor[c].sampler = m_alloc->acquireSampler(info);
      dutil.setObjectName(m_res.descriptor[c].sampler, "G-Sampler");
    }
  }

  {  // Depth buffer
    VkImageCreateInfo info = nvvk::makeImage2DCreateInfo(m_imageSize, m_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_res.gBufferDepth = m_alloc->createImage(info);
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
    auto*             cmd = cpool.createCommandBuffer();
    for(uint32_t c = 0; c < num_color; c++)
    {
      nvvk::cmdBarrierImageLayout(cmd, m_res.gBufferColor[c].image, VK_IMAGE_LAYOUT_UNDEFINED, layout);
      m_res.descriptor[c].imageLayout = layout;
    }
    cpool.submitAndWait(cmd);
  }

  // Descriptor Set for ImGUI
  if((ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().BackendPlatformUserData != nullptr)
  {
    for(auto& desc : m_res.descriptor)
    {
      m_descriptorSet.push_back(ImGui_ImplVulkan_AddTexture(desc.sampler, desc.imageView, layout));
    }
  }
}

nvvkhl::GBuffer::~GBuffer()
{
  // Destroy the resources in the next frame.
  // If submitResourceFree() throws an out-of-memory exception, avoid early
  // program termination.
  try
  {
    nvvkhl::Application::submitResourceFree([r = m_res, alloc = m_alloc, d = m_device] {
      auto bd = r.gBufferDepth;
      for(auto bc : r.gBufferColor)
        alloc->destroy(bc);
      alloc->destroy(bd);
      vkDestroyImageView(d, r.depthView, nullptr);
      for(const auto& desc : r.descriptor)
      {
        vkDestroyImageView(d, desc.imageView, nullptr);
        alloc->releaseSampler(desc.sampler);
      }
    });
  }
  catch(const std::exception& /* e */)
  {
    assert(!"Failed to queue resources!");
  }
}
