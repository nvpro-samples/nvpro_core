/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2020-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <vulkan/vulkan_core.h>

#include <assert.h>
#include <float.h>
#include <functional>
#include <string.h>  //memcmp
#include <unordered_map>
#include <vector>

#include "nvh/container_utils.hpp"

namespace nvvk {
//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::SamplerPool

  This nvvk::SamplerPool class manages unique VkSampler objects. To minimize the total
  number of sampler objects, this class ensures that identical configurations
  return the same sampler

  Example :
  \code{.cpp}
  nvvk::SamplerPool pool(device);

  for (auto it : textures) {
    VkSamplerCreateInfo info = {...};

    // acquire ensures we create the minimal subset of samplers
    it.sampler = pool.acquireSampler(info);
  }

  // you can manage releases individually, or just use deinit/destructor of pool
  for (auto it : textures) {
    pool.releaseSampler(it.sampler);
  }
  \endcode

  - makeSamplerCreateInfo : aids for sampler creation

*/

class SamplerPool
{
public:
  SamplerPool(SamplerPool const&) = delete;
  SamplerPool& operator=(SamplerPool const&) = delete;

  SamplerPool() {}
  SamplerPool(VkDevice device) { init(device); }
  ~SamplerPool() { deinit(); }

  void init(VkDevice device) { m_device = device; }
  void deinit();

  // creates a new sampler or re-uses an existing one with ref-count
  // createInfo may contain VkSamplerReductionModeCreateInfo and VkSamplerYcbcrConversionCreateInfo
  VkSampler acquireSampler(const VkSamplerCreateInfo& createInfo);

  // decrements ref-count and destroys sampler if possible
  void releaseSampler(VkSampler sampler);

private:
  struct SamplerState
  {
    VkSamplerCreateInfo                createInfo;
    VkSamplerReductionModeCreateInfo   reduction;
    VkSamplerYcbcrConversionCreateInfo ycbr;

    SamplerState() { memset(this, 0, sizeof(SamplerState)); }

    bool operator==(const SamplerState& other) const { return memcmp(this, &other, sizeof(SamplerState)) == 0; }
  };

  

  struct Chain
  {
    VkStructureType sType;
    const Chain*    pNext;
  };

  struct Entry
  {
    VkSampler    sampler       = nullptr;
    uint32_t     nextFreeIndex = ~0;
    uint32_t     refCount      = 0;
    SamplerState state;
  };

  VkDevice           m_device    = nullptr;
  uint32_t           m_freeIndex = ~0;
  std::vector<Entry> m_entries;

  std::unordered_map<SamplerState, uint32_t, nvh::HashAligned32<SamplerState>> m_stateMap;
  std::unordered_map<VkSampler, uint32_t>             m_samplerMap;
};

VkSamplerCreateInfo makeSamplerCreateInfo(VkFilter             magFilter        = VK_FILTER_LINEAR,
                                          VkFilter             minFilter        = VK_FILTER_LINEAR,
                                          VkSamplerAddressMode addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                          VkSamplerAddressMode addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                          VkSamplerAddressMode addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                          VkBool32             anisotropyEnable = VK_FALSE,
                                          float                maxAnisotropy    = 16,
                                          VkSamplerMipmapMode  mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                          float                minLod           = 0.0f,
                                          float                maxLod           = FLT_MAX,
                                          float                mipLodBias       = 0.0f,
                                          VkBool32             compareEnable    = VK_FALSE,
                                          VkCompareOp          compareOp        = VK_COMPARE_OP_ALWAYS,
                                          VkBorderColor        borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                          VkBool32             unnormalizedCoordinates = VK_FALSE);

}  // namespace nvvk
