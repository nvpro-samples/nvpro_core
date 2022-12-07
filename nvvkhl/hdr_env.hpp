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


#pragma once
//////////////////////////////////////////////////////////////////////////


#include <array>
#include <vector>

#include "nvvk/debug_util_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"

namespace nvvk {
class Context;
}

namespace nvvkhl {

//--------------------------------------------------------------------------------------------------
// Load an environment image (HDR) and create an acceleration structure for
// important light sampling.
class HdrEnv
{
public:
  HdrEnv() = default;
  HdrEnv(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator, uint32_t queueFamilyIndex = 0U);
  ~HdrEnv() { destroy(); }

  void setup(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t familyIndex, nvvk::ResourceAllocator* allocator);
  void loadEnvironment(const std::string& hrdImage);
  void destroy();

  float getIntegral() const { return m_integral; }
  float getAverage() const { return m_average; }
  bool  isValid() const { return m_valid; }

  inline VkDescriptorSetLayout getDescriptorSetLayout() { return m_descSetLayout; }  // HDR + importance sampling
  inline VkDescriptorSet       getDescriptorSet() { return m_descSet; }
  const nvvk::Texture&         getHdrTexture() { return m_texHdr; }  // The loaded HDR texture
  VkExtent2D                   getHdrImageSize() { return m_hdrImageSize; }

private:
  VkDevice                 m_device{VK_NULL_HANDLE};
  uint32_t                 m_familyIndex{0};
  nvvk::ResourceAllocator* m_alloc{nullptr};
  nvvk::DebugUtil          m_debug;

  float      m_integral{1.F};
  float      m_average{1.F};
  bool       m_valid{false};
  VkExtent2D m_hdrImageSize{1,1};

  // Resources
  nvvk::Texture         m_texHdr;
  nvvk::Buffer          m_accelImpSmpl;
  VkDescriptorPool      m_descPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_descSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet       m_descSet{VK_NULL_HANDLE};


  void createDescriptorSetLayout();
};

}  // namespace nvvkhl
