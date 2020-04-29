/* Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
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

#include <vulkan/vulkan_core.h>

#include <assert.h>
#include <functional>
#include <unordered_map>
#include <vector>

namespace nvvk {
//////////////////////////////////////////////////////////////////////////
/**
  # nvvk::SamplerPool

  This class manages unique VkSampler objects. To minimize the total
  number of sampler objects, this class ensures that identical configurations
  return the same sampler

  Example :
  ~~~C++
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
  ~~~

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
}  // namespace nvvk
