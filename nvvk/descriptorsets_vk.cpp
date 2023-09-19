/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "descriptorsets_vk.hpp"

namespace nvvk {

//////////////////////////////////////////////////////////////////////////

void DescriptorSetContainer::init(VkDevice device)
{
  assert(m_device == VK_NULL_HANDLE);
  m_device = device;
}

void DescriptorSetContainer::setBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
  m_bindings.setBindings(bindings);
}

void DescriptorSetContainer::addBinding(uint32_t           binding,
                                        VkDescriptorType   descriptorType,
                                        uint32_t           descriptorCount,
                                        VkShaderStageFlags stageFlags,
                                        const VkSampler*   pImmutableSamplers /*= nullptr*/)
{
  m_bindings.addBinding(binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers);
}

void DescriptorSetContainer::addBinding(VkDescriptorSetLayoutBinding binding)
{
  m_bindings.addBinding(binding);
}

void DescriptorSetContainer::setBindingFlags(uint32_t binding, VkDescriptorBindingFlags bindingFlag)
{
  m_bindings.setBindingFlags(binding, bindingFlag);
}

VkDescriptorSetLayout DescriptorSetContainer::initLayout(VkDescriptorSetLayoutCreateFlags flags /*= 0*/, DescriptorSupport supportFlags)
{
  assert(m_layout == VK_NULL_HANDLE);

  m_layout = m_bindings.createLayout(m_device, flags, supportFlags);
  return m_layout;
}

VkDescriptorPool DescriptorSetContainer::initPool(uint32_t numAllocatedSets)
{
  assert(m_pool == VK_NULL_HANDLE);
  assert(m_layout);

  m_pool = m_bindings.createPool(m_device, numAllocatedSets);
  allocateDescriptorSets(m_device, m_pool, m_layout, numAllocatedSets, m_descriptorSets);
  return m_pool;
}

VkPipelineLayout DescriptorSetContainer::initPipeLayout(uint32_t                    numRanges /*= 0*/,
                                                        const VkPushConstantRange*  ranges /*= nullptr*/,
                                                        VkPipelineLayoutCreateFlags flags /*= 0*/)
{
  assert(m_pipelineLayout == VK_NULL_HANDLE);
  assert(m_layout);

  VkResult                   result;
  VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layoutCreateInfo.setLayoutCount             = 1;
  layoutCreateInfo.pSetLayouts                = &m_layout;
  layoutCreateInfo.pushConstantRangeCount     = numRanges;
  layoutCreateInfo.pPushConstantRanges        = ranges;
  layoutCreateInfo.flags                      = flags;

  result = vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr, &m_pipelineLayout);
  assert(result == VK_SUCCESS);
  return m_pipelineLayout;
}

void DescriptorSetContainer::deinitPool()
{
  if(!m_descriptorSets.empty())
  {
    m_descriptorSets.clear();
  }

  if(m_pool)
  {
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    m_pool = VK_NULL_HANDLE;
  }
}

void DescriptorSetContainer::deinitLayout()
{
  if(m_pipelineLayout)
  {
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
  }

  if(m_layout)
  {
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
    m_layout = VK_NULL_HANDLE;
  }
}

void DescriptorSetContainer::deinit()
{
  deinitLayout();
  deinitPool();
  m_bindings.clear();
  m_device = VK_NULL_HANDLE;
}

VkDescriptorSet DescriptorSetContainer::getSet(uint32_t dstSetIdx /*= 0*/) const
{
  if(m_descriptorSets.empty())
  {
    return {};
  }

  return m_descriptorSets[dstSetIdx];
}

//////////////////////////////////////////////////////////////////////////

VkDescriptorSetLayout DescriptorSetBindings::createLayout(VkDevice device, VkDescriptorSetLayoutCreateFlags flags, DescriptorSupport supportFlags)
{
  VkResult                                    result;
  VkDescriptorSetLayoutBindingFlagsCreateInfo bindingsInfo = {
      isSet(supportFlags, DescriptorSupport::CORE_1_2) ? VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO :
                                                         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT};

  // Pad binding flags to match bindings if any exist
  if(!m_bindingFlags.empty() && m_bindingFlags.size() <= m_bindings.size())
  {
    m_bindingFlags.resize(m_bindings.size(), 0);
  }

  bindingsInfo.bindingCount  = uint32_t(m_bindingFlags.size());
  bindingsInfo.pBindingFlags = m_bindingFlags.data();

  VkDescriptorSetLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  createInfo.bindingCount                    = uint32_t(m_bindings.size());
  createInfo.pBindings                       = m_bindings.data();
  createInfo.flags                           = flags;
  createInfo.pNext =
      m_bindingFlags.empty() && !(isAnySet(supportFlags, (DescriptorSupport::CORE_1_2 | DescriptorSupport::INDEXING_EXT))) ?
          nullptr :
          &bindingsInfo;

  VkDescriptorSetLayout descriptorSetLayout;
  result = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout);
  assert(result == VK_SUCCESS);

  return descriptorSetLayout;
}

void DescriptorSetBindings::addRequiredPoolSizes(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t numSets) const
{
  for(auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it)
  {
    // Bindings can have a zero descriptor count, used for the layout, but don't reserve storage for them.
    if(it->descriptorCount == 0)
    {
      continue;
    }

    bool found = false;
    for(auto itpool = poolSizes.begin(); itpool != poolSizes.end(); ++itpool)
    {
      if(itpool->type == it->descriptorType)
      {
        itpool->descriptorCount += it->descriptorCount * numSets;
        found = true;
        break;
      }
    }
    if(!found)
    {
      VkDescriptorPoolSize poolSize{};
      poolSize.type            = it->descriptorType;
      poolSize.descriptorCount = it->descriptorCount * numSets;
      poolSizes.push_back(poolSize);
    }
  }
}

VkDescriptorPool DescriptorSetBindings::createPool(VkDevice device, uint32_t maxSets /*= 1*/, VkDescriptorPoolCreateFlags flags /*= 0*/) const
{
  VkResult result;

  // setup poolsizes for each descriptorType
  std::vector<VkDescriptorPoolSize> poolSizes;
  addRequiredPoolSizes(poolSizes, maxSets);

  VkDescriptorPool           descrPool;
  VkDescriptorPoolCreateInfo descrPoolInfo = {};
  descrPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descrPoolInfo.pNext                      = nullptr;
  descrPoolInfo.maxSets                    = maxSets;
  descrPoolInfo.poolSizeCount              = uint32_t(poolSizes.size());
  descrPoolInfo.pPoolSizes                 = poolSizes.data();
  descrPoolInfo.flags                      = flags;

  // scene pool
  result = vkCreateDescriptorPool(device, &descrPoolInfo, nullptr, &descrPool);
  assert(result == VK_SUCCESS);
  return descrPool;
}


void DescriptorSetBindings::setBindingFlags(uint32_t binding, VkDescriptorBindingFlags bindingFlag)
{
  for(size_t i = 0; i < m_bindings.size(); i++)
  {
    if(m_bindings[i].binding == binding)
    {
      if(m_bindingFlags.size() <= m_bindings.size())
      {
        m_bindingFlags.resize(m_bindings.size(), 0);
      }
      m_bindingFlags[i] = bindingFlag;
      return;
    }
  }
  assert(0 && "binding not found");
}

VkDescriptorType DescriptorSetBindings::getType(uint32_t binding) const
{
  for(size_t i = 0; i < m_bindings.size(); i++)
  {
    if(m_bindings[i].binding == binding)
    {
      return m_bindings[i].descriptorType;
    }
  }
  assert(0 && "binding not found");
  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

uint32_t DescriptorSetBindings::getCount(uint32_t binding) const
{
  for(size_t i = 0; i < m_bindings.size(); i++)
  {
    if(m_bindings[i].binding == binding)
    {
      return m_bindings[i].descriptorCount;
    }
  }
  assert(0 && "binding not found");
  return ~0;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement) const
{
  VkWriteDescriptorSet writeSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  writeSet.descriptorType       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
  for(size_t i = 0; i < m_bindings.size(); i++)
  {
    if(m_bindings[i].binding == dstBinding)
    {
      writeSet.descriptorCount = 1;
      writeSet.descriptorType  = m_bindings[i].descriptorType;
      writeSet.dstBinding      = dstBinding;
      writeSet.dstSet          = dstSet;
      writeSet.dstArrayElement = arrayElement;
      return writeSet;
    }
  }
  assert(0 && "binding not found");
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet dstSet, uint32_t dstBinding) const
{
  VkWriteDescriptorSet writeSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  writeSet.descriptorType       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
  for(size_t i = 0; i < m_bindings.size(); i++)
  {
    if(m_bindings[i].binding == dstBinding)
    {
      writeSet.descriptorCount = m_bindings[i].descriptorCount;
      writeSet.descriptorType  = m_bindings[i].descriptorType;
      writeSet.dstBinding      = dstBinding;
      writeSet.dstSet          = dstSet;
      writeSet.dstArrayElement = 0;
      return writeSet;
    }
  }
  assert(0 && "binding not found");
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet              dstSet,
                                                      uint32_t                     dstBinding,
                                                      const VkDescriptorImageInfo* pImageInfo,
                                                      uint32_t                     arrayElement) const
{
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

  writeSet.pImageInfo = pImageInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet               dstSet,
                                                      uint32_t                      dstBinding,
                                                      const VkDescriptorBufferInfo* pBufferInfo,
                                                      uint32_t                      arrayElement) const
{
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

  writeSet.pBufferInfo = pBufferInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet     dstSet,
                                                      uint32_t            dstBinding,
                                                      const VkBufferView* pTexelBufferView,
                                                      uint32_t            arrayElement) const
{
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);

  writeSet.pTexelBufferView = pTexelBufferView;
  return writeSet;
}

#if VK_NV_ray_tracing
VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet                                    dstSet,
                                                      uint32_t                                           dstBinding,
                                                      const VkWriteDescriptorSetAccelerationStructureNV* pAccel,
                                                      uint32_t arrayElement) const
{
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);

  writeSet.pNext = pAccel;
  return writeSet;
}
#endif
#if VK_KHR_acceleration_structure
VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet                                     dstSet,
                                                      uint32_t                                            dstBinding,
                                                      const VkWriteDescriptorSetAccelerationStructureKHR* pAccel,
                                                      uint32_t arrayElement) const
{
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

  writeSet.pNext = pAccel;
  return writeSet;
}
#endif

#if VK_EXT_inline_uniform_block
VkWriteDescriptorSet DescriptorSetBindings::makeWrite(VkDescriptorSet                                  dstSet,
                                                      uint32_t                                         dstBinding,
                                                      const VkWriteDescriptorSetInlineUniformBlockEXT* pInline,
                                                      uint32_t arrayElement) const
{
  VkWriteDescriptorSet writeSet = makeWrite(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT);

  writeSet.pNext = pInline;
  return writeSet;
}
#endif
VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet              dstSet,
                                                           uint32_t                     dstBinding,
                                                           const VkDescriptorImageInfo* pImageInfo) const
{
  VkWriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

  writeSet.pImageInfo = pImageInfo;
  assert(writeSet.descriptorCount > 0);  // Can have a zero descriptors in the descriptorset layout, but can't write zero items.
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet               dstSet,
                                                           uint32_t                      dstBinding,
                                                           const VkDescriptorBufferInfo* pBufferInfo) const
{
  VkWriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

  writeSet.pBufferInfo = pBufferInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
{
  VkWriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);

  writeSet.pTexelBufferView = pTexelBufferView;
  return writeSet;
}

#if VK_NV_ray_tracing
VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet dstSet,
                                                           uint32_t        dstBinding,
                                                           const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const
{
  VkWriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);

  writeSet.pNext = pAccel;
  return writeSet;
}
#endif
#if VK_KHR_acceleration_structure
VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet dstSet,
                                                           uint32_t        dstBinding,
                                                           const VkWriteDescriptorSetAccelerationStructureKHR* pAccel) const
{
  VkWriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

  writeSet.pNext = pAccel;
  return writeSet;
}
#endif
#if VK_EXT_inline_uniform_block
VkWriteDescriptorSet DescriptorSetBindings::makeWriteArray(VkDescriptorSet                                  dstSet,
                                                           uint32_t                                         dstBinding,
                                                           const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  VkWriteDescriptorSet writeSet = makeWriteArray(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT);

  writeSet.pNext = pInline;
  return writeSet;
}
#endif

}  // namespace nvvk
