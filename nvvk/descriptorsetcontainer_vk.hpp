/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

#include <assert.h>
#include <platform.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "makers_vk.hpp"
#include "deviceutils_vk.hpp"

namespace nvvk

{

template <uint32_t DSETS, uint32_t PIPELAYOUTS = 1>
struct DescriptorSetContainer
{
  /*
    Utility class to manage VkDescriptorSetLayout VkDescriptorPool and VkDescriptorSets.
    It also can store a fixed number of pipelineLayouts, which typically reference
    all or a subset of the VkDescriptorSetLayout within.

    Store a fixed number of DSETS many VkDescriptorSetLayouts
    and one pool for each such layout.
    Also contains reflection code to ease creation VkWriteDescriptorSet when
    updating descriptorsets. All updates will go to the descriptorset stored
    within the container.

    Typical usage for DSETS=2

    container.init(device)

    auto& bindings0 = container.descriptorBindings[0];
    binding0.push_back({...});
    binding0.push_back({...});
    ...
    auto& bindings1 = container.descriptorBindings[1];
    binding1.push_back({...});

    then 

    container.initSetLayout(0);   // uses descriptorBindings[0]
    container.initSetLayout(1);   // uses descriptorBindings[1]

    container.initPipeLayout(0);  // uses {descriptorSetLayout[0], descriptorSetLayout[1]}

    container.initPoolAndSets(0, maxSetsFor0);
    container.initPoolAndSets(1, maxSetsFor1);

    std::vector<vkWriteDescriptorSet> updateSets;
    updateSets.push_back( container.getWriteDescriptorSet(0, dstSetIndex, dstBinding...));
    updateSets.push_back( container.getWriteDescriptorSet(0, dstSetIndex, dstBinding...));
    updateSets.push_back( container.getWriteDescriptorSet(1, dstSetIndex, dstBinding...));
  
  */


  DeviceUtils                               deviceUtils;
  VkPipelineLayout                          pipelineLayouts[PIPELAYOUTS] = {};
  VkDescriptorSetLayout                     descriptorSetLayout[DSETS]   = {};
  VkDescriptorPool                          descriptorPools[DSETS]       = {};
  std::vector<VkDescriptorSet>              descriptorSets[DSETS]        = {};
  std::vector<VkDescriptorSetLayoutBinding> descriptorBindings[DSETS]    = {};

  void init(VkDevice device, const VkAllocationCallbacks* pAllocator = nullptr)
  {
    deviceUtils = DeviceUtils(device, pAllocator);
  }

  void init(const DeviceUtils& utils) { deviceUtils = utils; }

  void initSetLayout(uint32_t dset, VkDescriptorSetLayoutCreateFlags flags = 0)
  {
    assert(deviceUtils.m_device);

    descriptorSetLayout[dset] = deviceUtils.createDescriptorSetLayout((uint32_t)descriptorBindings[dset].size(),
                                                                      descriptorBindings[dset].data(), flags);
  }

  void initPipeLayout(uint32_t pipe, uint32_t numRanges = 0, const VkPushConstantRange* ranges = nullptr, VkPipelineLayoutCreateFlags flags = 0)
  {
    assert(deviceUtils.m_device);

    pipelineLayouts[pipe] = deviceUtils.createPipelineLayout(DSETS, descriptorSetLayout, numRanges, ranges, flags);
  }

  void initPoolAndSets(uint32_t dset, uint32_t maxSets, size_t poolSizeCount, const VkDescriptorPoolSize* poolSizes)
  {
    assert(deviceUtils.m_device);

    descriptorSets[dset].resize(maxSets);
    deviceUtils.createDescriptorPoolAndSets(maxSets, poolSizeCount, poolSizes, descriptorSetLayout[dset],
                                            descriptorPools[dset], descriptorSets[dset].data());
  }

  void initPoolAndSets(uint32_t dset, uint32_t maxSets)
  {
    assert(deviceUtils.m_device);
    assert(!descriptorBindings[dset].empty());

    descriptorSets[dset].resize(maxSets);

    // setup poolsizes for each descriptorType
    std::vector<VkDescriptorPoolSize> poolSizes;
    for(auto it = descriptorBindings[dset].cbegin(); it != descriptorBindings[dset].cend(); ++it)
    {
      bool found = false;
      for(auto itpool = poolSizes.begin(); itpool != poolSizes.end(); ++itpool)
      {
        if(itpool->type == it->descriptorType)
        {
          itpool->descriptorCount += it->descriptorCount;
          found = true;
          break;
        }
      }
      if(!found)
      {
        VkDescriptorPoolSize poolSize;
        poolSize.type            = it->descriptorType;
        poolSize.descriptorCount = it->descriptorCount;
        poolSizes.push_back(poolSize);
      }
    }
    deviceUtils.createDescriptorPoolAndSets(maxSets, (uint32_t)poolSizes.size(), poolSizes.data(),
                                            descriptorSetLayout[dset], descriptorPools[dset], descriptorSets[dset].data());
  }

  void deinitPools()
  {
    assert(deviceUtils.m_device);

    for(uint32_t i = 0; i < DSETS; i++)
    {
      if(descriptorPools[i])
      {
        vkDestroyDescriptorPool(deviceUtils.m_device, descriptorPools[i], deviceUtils.m_allocator);
        descriptorSets[i].clear();
        descriptorPools[i] = nullptr;
      }
    }
  }

  void deinitPool(uint32_t dset)
  {
    assert(deviceUtils.m_device);

    if(descriptorPools[dset])
    {
      vkDestroyDescriptorPool(deviceUtils.m_device, descriptorPools[dset], deviceUtils.m_allocator);
      descriptorSets[dset].clear();
      descriptorPools[dset] = nullptr;
    }
  }

  void deinitLayouts()
  {
    assert(deviceUtils.m_device);

    for(uint32_t i = 0; i < PIPELAYOUTS; i++)
    {
      if(pipelineLayouts[i])
      {
        vkDestroyPipelineLayout(deviceUtils.m_device, pipelineLayouts[i], deviceUtils.m_allocator);
        pipelineLayouts[i] = nullptr;
      }
    }
    for(uint32_t i = 0; i < DSETS; i++)
    {
      if(descriptorSetLayout[i])
      {
        vkDestroyDescriptorSetLayout(deviceUtils.m_device, descriptorSetLayout[i], deviceUtils.m_allocator);
        descriptorSetLayout[i] = nullptr;
      }
      descriptorBindings[i].clear();
    }
  }

  void deinit()
  {
    deinitLayouts();
    deinitPools();
  }

  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, pImageInfo);
  }
  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, pBufferInfo);
  }
  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, pTexelBufferView);
  }
  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, const void* pNext) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, pNext);
  }

  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t                     dset,
                                             uint32_t                     dstSet,
                                             uint32_t                     dstBinding,
                                             uint32_t                     arrayElement,
                                             const VkDescriptorImageInfo* pImageInfo) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, arrayElement, pImageInfo);
  }
  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t                      dset,
                                             uint32_t                      dstSet,
                                             uint32_t                      dstBinding,
                                             uint32_t                      arrayElement,
                                             const VkDescriptorBufferInfo* pBufferInfo) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, arrayElement, pBufferInfo);
  }
  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, arrayElement, pTexelBufferView);
  }
  VkWriteDescriptorSet getWriteDescriptorSet(uint32_t dset, uint32_t dstSet, uint32_t dstBinding, uint32_t arrayElement, const void* pNext) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(),
                                  descriptorSets[dset][dstSet], dstBinding, arrayElement, pNext);
  }

  VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pImageInfo);
  }
  VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pBufferInfo);
  }
  VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pTexelBufferView);
  }
  VkWriteDescriptorSet getPushWriteDescriptorSet(uint32_t dset, uint32_t dstBinding, const void* pNext) const
  {
    return makeWriteDescriptorSet(descriptorBindings[dset].size(), descriptorBindings[dset].data(), nullptr, dstBinding, pNext);
  }

  const VkDescriptorSet* getSets(uint32_t dset = 0) const { return descriptorSets[dset].data(); }

  VkPipelineLayout getPipeLayout(uint32_t pipe = 0) const { return pipelineLayouts[pipe]; }

  size_t getSetsCount(uint32_t dset = 0) const { return descriptorSets[dset].size(); }
};

}  // namespace nvvk
