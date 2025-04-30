/*
 * Copyright (c) 2020-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2020-2025, NVIDIA CORPORATION.
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
#include <mutex>

#include "nvh/container_utils.hpp"

namespace nvvk {
//////////////////////////////////////////////////////////////////////////
/** @DOC_START
  # class nvvk::SamplerPool

  This nvvk::SamplerPool class manages unique VkSampler objects. To minimize the total
  number of sampler objects, this class ensures that identical configurations
  return the same sampler

  Example :
  ```cpp
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
  ```

  - makeSamplerCreateInfo : aids for sampler creation

@DOC_END */

class SamplerPool
{
public:
  SamplerPool(SamplerPool const&)            = delete;
  SamplerPool& operator=(SamplerPool const&) = delete;

  SamplerPool() {}
  SamplerPool(VkDevice device) { init(device); }
  ~SamplerPool() { deinit(); }

  void init(VkDevice device) { m_device = device; }
  void deinit();


  // these two functions are thread-safe, protected by an internal lock

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

    bool operator==(const SamplerState& other) const
    {
      return other.createInfo.flags == createInfo.flags && other.createInfo.magFilter == createInfo.magFilter
             && other.createInfo.minFilter == createInfo.minFilter && other.createInfo.mipmapMode == createInfo.mipmapMode
             && other.createInfo.addressModeU == createInfo.addressModeU && other.createInfo.addressModeV == createInfo.addressModeV
             && other.createInfo.addressModeW == createInfo.addressModeW && other.createInfo.mipLodBias == createInfo.mipLodBias
             && other.createInfo.anisotropyEnable == createInfo.anisotropyEnable
             && other.createInfo.maxAnisotropy == createInfo.maxAnisotropy
             && other.createInfo.compareEnable == createInfo.compareEnable
             && other.createInfo.compareOp == createInfo.compareOp && other.createInfo.minLod == createInfo.minLod
             && other.createInfo.maxLod == createInfo.maxLod && other.createInfo.borderColor == createInfo.borderColor
             && other.createInfo.unnormalizedCoordinates == createInfo.unnormalizedCoordinates
             && other.reduction.reductionMode == reduction.reductionMode && other.ycbr.format == ycbr.format
             && other.ycbr.ycbcrModel == ycbr.ycbcrModel && other.ycbr.ycbcrRange == ycbr.ycbcrRange
             && other.ycbr.components.r == ycbr.components.r && other.ycbr.components.g == ycbr.components.g
             && other.ycbr.components.b == ycbr.components.b && other.ycbr.components.a == ycbr.components.a
             && other.ycbr.xChromaOffset == ycbr.xChromaOffset && other.ycbr.yChromaOffset == ycbr.yChromaOffset
             && other.ycbr.chromaFilter == ycbr.chromaFilter
             && other.ycbr.forceExplicitReconstruction == ycbr.forceExplicitReconstruction;
    }
  };

  struct Hash_fn
  {
    std::size_t operator()(const SamplerState& s) const
    {
      return nvh::hashVal(s.createInfo.flags, s.createInfo.magFilter, s.createInfo.minFilter, s.createInfo.mipmapMode,
                          s.createInfo.addressModeU, s.createInfo.addressModeV, s.createInfo.addressModeW,
                          s.createInfo.mipLodBias, s.createInfo.anisotropyEnable, s.createInfo.maxAnisotropy,
                          s.createInfo.compareEnable, s.createInfo.compareOp, s.createInfo.minLod, s.createInfo.maxLod,
                          s.createInfo.borderColor, s.createInfo.unnormalizedCoordinates, s.reduction.reductionMode,
                          s.ycbr.format, s.ycbr.ycbcrModel, s.ycbr.ycbcrRange, s.ycbr.components.r, s.ycbr.components.g,
                          s.ycbr.components.b, s.ycbr.components.a, s.ycbr.xChromaOffset, s.ycbr.yChromaOffset,
                          s.ycbr.chromaFilter, s.ycbr.forceExplicitReconstruction);
    }
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

  std::mutex         m_mutex;
  VkDevice           m_device    = nullptr;
  uint32_t           m_freeIndex = ~0;
  std::vector<Entry> m_entries;

  std::unordered_map<SamplerState, uint32_t, Hash_fn> m_stateMap;
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
                                          float                maxLod           = VK_LOD_CLAMP_NONE,
                                          float                mipLodBias       = 0.0f,
                                          VkBool32             compareEnable    = VK_FALSE,
                                          VkCompareOp          compareOp        = VK_COMPARE_OP_ALWAYS,
                                          VkBorderColor        borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                          VkBool32             unnormalizedCoordinates = VK_FALSE);

}  // namespace nvvk
