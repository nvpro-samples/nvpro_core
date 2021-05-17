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


#include "samplers_vk.hpp"

namespace nvvk {
//////////////////////////////////////////////////////////////////////////

void SamplerPool::deinit()
{
  if(!m_device)
    return;

  for(auto it : m_entries)
  {
    if(it.sampler)
    {
      vkDestroySampler(m_device, it.sampler, nullptr);
    }
  }

  m_freeIndex = ~0;
  m_entries.clear();
  m_samplerMap.clear();
  m_stateMap.clear();
  m_device = nullptr;
}

VkSampler SamplerPool::acquireSampler(const VkSamplerCreateInfo& createInfo)
{
  SamplerState state;
  state.createInfo = createInfo;

  const Chain* ext = (const Chain*)createInfo.pNext;
  while(ext)
  {
    switch(ext->sType)
    {
      case VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO:
        state.reduction = *(const VkSamplerReductionModeCreateInfo*)ext;
        break;
      case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO:
        state.ycbr = *(const VkSamplerYcbcrConversionCreateInfo*)ext;
        break;
      default:
        assert(0 && "unsupported sampler create");
    }
    ext = ext->pNext;
  }
  // always remove pointers for comparison lookup
  state.createInfo.pNext = nullptr;
  state.reduction.pNext  = nullptr;
  state.ycbr.pNext       = nullptr;

  auto it = m_stateMap.find(state);
  if(it == m_stateMap.end())
  {
    uint32_t index = 0;
    if(m_freeIndex != ~0)
    {
      index       = m_freeIndex;
      m_freeIndex = m_entries[index].nextFreeIndex;
    }
    else
    {
      index = (uint32_t)m_entries.size();
      m_entries.resize(m_entries.size() + 1);
    }

    VkSampler sampler;
    VkResult  result = vkCreateSampler(m_device, &createInfo, nullptr, &sampler);
    assert(result == VK_SUCCESS);

    m_entries[index].refCount = 1;
    m_entries[index].sampler  = sampler;
    m_entries[index].state    = state;

    m_stateMap.insert({state, index});
    m_samplerMap.insert({sampler, index});

    return sampler;
  }
  else
  {
    m_entries[it->second].refCount++;
    return m_entries[it->second].sampler;
  }
}

void SamplerPool::releaseSampler(VkSampler sampler)
{
  auto it = m_samplerMap.find(sampler);
  assert(it != m_samplerMap.end());

  uint32_t index = it->second;
  Entry&   entry = m_entries[index];

  assert(entry.sampler == sampler);
  assert(entry.refCount);

  entry.refCount--;

  if(!entry.refCount)
  {
    vkDestroySampler(m_device, sampler, nullptr);
    entry.sampler       = nullptr;
    entry.nextFreeIndex = m_freeIndex;
    m_freeIndex         = index;

    m_stateMap.erase(entry.state);
    m_samplerMap.erase(sampler);
  }
}

VkSamplerCreateInfo makeSamplerCreateInfo(VkFilter             magFilter,
                                          VkFilter             minFilter,
                                          VkSamplerAddressMode addressModeU,
                                          VkSamplerAddressMode addressModeV,
                                          VkSamplerAddressMode addressModeW,
                                          VkBool32             anisotropyEnable,
                                          float                maxAnisotropy,
                                          VkSamplerMipmapMode  mipmapMode,
                                          float                minLod,
                                          float                maxLod,
                                          float                mipLodBias,
                                          VkBool32             compareEnable,
                                          VkCompareOp          compareOp,
                                          VkBorderColor        borderColor,
                                          VkBool32             unnormalizedCoordinates)
{
  VkSamplerCreateInfo samplerInfo     = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.flags                   = 0;
  samplerInfo.pNext                   = nullptr;
  samplerInfo.magFilter               = magFilter;
  samplerInfo.minFilter               = minFilter;
  samplerInfo.mipmapMode              = mipmapMode;
  samplerInfo.addressModeU            = addressModeU;
  samplerInfo.addressModeV            = addressModeV;
  samplerInfo.addressModeW            = addressModeW;
  samplerInfo.anisotropyEnable        = anisotropyEnable;
  samplerInfo.maxAnisotropy           = maxAnisotropy;
  samplerInfo.borderColor             = borderColor;
  samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates;
  samplerInfo.compareEnable           = compareEnable;
  samplerInfo.compareOp               = compareOp;
  return samplerInfo;
}

}  // namespace nvvk
