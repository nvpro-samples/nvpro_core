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

  struct Hash_fn
  {
    std::size_t operator()(const SamplerState& s) const
    {
      std::hash<uint32_t> hasher;
      const uint32_t*     data = (const uint32_t*)&s;
      size_t              seed = 0;
      for(size_t i = 0; i < sizeof(SamplerState) / sizeof(uint32_t); i++)
      {
        // https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
        seed ^= hasher(data[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
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
                                          float                maxLod           = FLT_MAX,
                                          float                mipLodBias       = 0.0f,
                                          VkBool32             compareEnable    = VK_FALSE,
                                          VkCompareOp          compareOp        = VK_COMPARE_OP_ALWAYS,
                                          VkBorderColor        borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                          VkBool32             unnormalizedCoordinates = VK_FALSE);

#ifdef VULKAN_HPP
inline vk::SamplerCreateInfo makeSamplerCreateInfo(vk::Filter             magFilter = vk::Filter::eLinear,
                                                   vk::Filter             minFilter = vk::Filter::eLinear,
                                                   vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eClampToEdge,
                                                   vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eClampToEdge,
                                                   vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eClampToEdge,
                                                   vk::Bool32             anisotropyEnable = VK_FALSE,
                                                   float                  maxAnisotropy    = 16,
                                                   vk::SamplerMipmapMode mipmapMode    = vk::SamplerMipmapMode::eLinear,
                                                   float                 minLod        = 0.0f,
                                                   float                 maxLod        = FLT_MAX,
                                                   float                 mipLodBias    = 0.0f,
                                                   vk::Bool32            compareEnable = VK_FALSE,
                                                   vk::CompareOp         compareOp     = vk::CompareOp::eAlways,
                                                   vk::BorderColor       borderColor = vk::BorderColor::eIntOpaqueBlack,
                                                   vk::Bool32            unnormalizedCoordinates = VK_FALSE)
{
  return makeSamplerCreateInfo(static_cast<VkFilter>(magFilter), static_cast<VkFilter>(minFilter),
                               static_cast<VkSamplerAddressMode>(addressModeU), static_cast<VkSamplerAddressMode>(addressModeV),
                               static_cast<VkSamplerAddressMode>(addressModeW), static_cast<VkBool32>(anisotropyEnable),
                               maxAnisotropy, static_cast<VkSamplerMipmapMode>(mipmapMode), minLod, maxLod, mipLodBias,
                               compareEnable, static_cast<VkCompareOp>(compareOp),
                               static_cast<VkBorderColor>(borderColor), static_cast<VkBool32>(unnormalizedCoordinates));
}
#endif

}  // namespace nvvk
