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

#include "descriptorsets_vk.hpp"

namespace nvvk {

  //////////////////////////////////////////////////////////////////////////

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice                            device,
                                                size_t                              numBindings,
                                                const VkDescriptorSetLayoutBinding* bindings,
                                                VkDescriptorSetLayoutCreateFlags    flags /*= 0*/)
{
  VkResult                        result;
  VkDescriptorSetLayoutCreateInfo descriptorSetEntry = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  descriptorSetEntry.bindingCount                    = uint32_t(numBindings);
  descriptorSetEntry.pBindings                       = bindings;
  descriptorSetEntry.flags                           = flags;

  VkDescriptorSetLayout descriptorSetLayout;
  result = vkCreateDescriptorSetLayout(device, &descriptorSetEntry, nullptr, &descriptorSetLayout);
  assert(result == VK_SUCCESS);

  return descriptorSetLayout;
}

VkDescriptorPool createDescriptorPool(VkDevice device, size_t poolSizeCount, const VkDescriptorPoolSize* poolSizes, uint32_t maxSets)
{
  VkResult result;

  VkDescriptorPool           descrPool;
  VkDescriptorPoolCreateInfo descrPoolInfo = {};
  descrPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descrPoolInfo.pNext                      = nullptr;
  descrPoolInfo.maxSets                    = maxSets;
  descrPoolInfo.poolSizeCount              = uint32_t(poolSizeCount);
  descrPoolInfo.pPoolSizes                 = poolSizes;

  // scene pool
  result = vkCreateDescriptorPool(device, &descrPoolInfo, nullptr, &descrPool);
  assert(result == VK_SUCCESS);
  return descrPool;
}

VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
  VkResult                    result;
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool              = pool;
  allocInfo.descriptorSetCount          = 1;
  allocInfo.pSetLayouts                 = &layout;

  VkDescriptorSet set;
  result = vkAllocateDescriptorSets(device, &allocInfo, &set);
  assert(result == VK_SUCCESS);
  return set;
}

void allocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, uint32_t count, std::vector<VkDescriptorSet>& sets)
{
  sets.resize(count);
  std::vector<VkDescriptorSetLayout> layouts(count, layout);

  VkResult                    result;
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool              = pool;
  allocInfo.descriptorSetCount          = count;
  allocInfo.pSetLayouts                 = layouts.data();

  result = vkAllocateDescriptorSets(device, &allocInfo, sets.data());
  assert(result == VK_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

void DescriptorSetContainer::init(VkDevice device)
{
  m_device    = device;
}

void DescriptorSetContainer::setBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
  m_reflection.setBindings(bindings);
}

void DescriptorSetContainer::addBinding(uint32_t           binding,
                                         VkDescriptorType   descriptorType,
                                         uint32_t           descriptorCount,
                                         VkShaderStageFlags stageFlags,
                                         const VkSampler*   pImmutableSamplers /*= nullptr*/)
{
  m_reflection.addBinding(binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers);
}

void DescriptorSetContainer::addBinding(VkDescriptorSetLayoutBinding binding)
{
  m_reflection.addBinding(binding);
}

VkDescriptorSetLayout DescriptorSetContainer::initLayout(VkDescriptorSetLayoutCreateFlags flags /*= 0*/)
{
  m_layout = m_reflection.createLayout(m_device, flags, nullptr);
  return m_layout;
}

VkDescriptorPool DescriptorSetContainer::initPool(uint32_t numAllocatedSets)
{
  m_pool = m_reflection.createPool(m_device, numAllocatedSets, nullptr);
  allocateDescriptorSets(m_device, m_pool, m_layout, numAllocatedSets, m_descriptorSets);
  return m_pool;
}

VkPipelineLayout DescriptorSetContainer::initPipeLayout(uint32_t                    numRanges /*= 0*/,
                                            const VkPushConstantRange*  ranges /*= nullptr*/,
                                            VkPipelineLayoutCreateFlags flags /*= 0*/)
{
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
  if (!m_descriptorSets.empty())
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
}

VkWriteDescriptorSet DescriptorSetContainer::getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const
{
  return m_reflection.getWrite(m_descriptorSets[dstSetIdx], dstBinding, pImageInfo);
}

VkWriteDescriptorSet DescriptorSetContainer::getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const
{
  return m_reflection.getWrite(m_descriptorSets[dstSetIdx], dstBinding, pBufferInfo);
}

VkWriteDescriptorSet DescriptorSetContainer::getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
{
  return m_reflection.getWrite(m_descriptorSets[dstSetIdx], dstBinding, pTexelBufferView);
}

VkWriteDescriptorSet DescriptorSetContainer::getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const void* pNext) const
{
  return m_reflection.getWrite(m_descriptorSets[dstSetIdx], dstBinding, pNext);
}

VkWriteDescriptorSet DescriptorSetContainer::getWrite(uint32_t                                           dstSetIdx,
                                                       uint32_t                                           dstBinding,
                                                       const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const
{
  return m_reflection.getWrite(m_descriptorSets[dstSetIdx], dstBinding, pAccel);
}

VkWriteDescriptorSet DescriptorSetContainer::getWrite(uint32_t                                         dstSetIdx,
                                                      uint32_t                                         dstBinding,
                                                      const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  return m_reflection.getWrite(m_descriptorSets[dstSetIdx], dstBinding, pInline);
}

VkWriteDescriptorSet DescriptorSetContainer::getWriteElement(uint32_t                     dstSetIdx,
                                                              uint32_t                     dstBinding,
                                                              uint32_t                     arrayElement,
                                                              const VkDescriptorImageInfo* pImageInfo) const
{
  return m_reflection.getWriteElement(m_descriptorSets[dstSetIdx], dstBinding, arrayElement, pImageInfo);
}

VkWriteDescriptorSet DescriptorSetContainer::getWriteElement(uint32_t            dstSetIdx,
                                                              uint32_t            dstBinding,
                                                              uint32_t            arrayElement,
                                                              const VkBufferView* pTexelBufferView) const
{
  return m_reflection.getWriteElement(m_descriptorSets[dstSetIdx], dstBinding, arrayElement, pTexelBufferView);
}

VkWriteDescriptorSet DescriptorSetContainer::getWriteElement(uint32_t dstSetIdx, uint32_t dstBinding, uint32_t arrayElement, const void* pNext) const
{
  return m_reflection.getWriteElement(m_descriptorSets[dstSetIdx], dstBinding, arrayElement, pNext);
}

VkWriteDescriptorSet DescriptorSetContainer::getWriteElement(uint32_t                      dstSetIdx,
                                                              uint32_t                      dstBinding,
                                                              uint32_t                      arrayElement,
                                                              const VkDescriptorBufferInfo* pBufferInfo) const
{
  return m_reflection.getWriteElement(m_descriptorSets[dstSetIdx], dstBinding, arrayElement, pBufferInfo);
}

VkWriteDescriptorSet DescriptorSetContainer::getWriteElement(uint32_t dstSetIdx,
                                                              uint32_t dstBinding,
                                                              uint32_t arrayElement,
                                                              const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const
{
  return m_reflection.getWriteElement(m_descriptorSets[dstSetIdx], dstBinding, arrayElement, pAccel);
}

VkWriteDescriptorSet DescriptorSetContainer::getWriteElement(uint32_t dstSetIdx,
                                                              uint32_t dstBinding,
                                                              uint32_t arrayElement,
                                                              const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  return m_reflection.getWriteElement(m_descriptorSets[dstSetIdx], dstBinding, arrayElement, pInline);
}

//////////////////////////////////////////////////////////////////////////

void DescriptorSetReflection::addBinding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stageFlags, const VkSampler* pImmutableSampler)
{
  m_bindings.push_back({binding, type, count, stageFlags, pImmutableSampler});
}

void DescriptorSetReflection::addBinding(VkDescriptorSetLayoutBinding binding)
{
  m_bindings.push_back(binding);
}

void DescriptorSetReflection::setBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
  m_bindings = bindings;
}

VkDescriptorSetLayout DescriptorSetReflection::createLayout(VkDevice                         device,
                                                            VkDescriptorSetLayoutCreateFlags flags,
                                                            const VkAllocationCallbacks*     pAllocator) const
{
  VkResult                        result;
  VkDescriptorSetLayoutCreateInfo descriptorSetEntry = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  descriptorSetEntry.bindingCount                    = uint32_t(m_bindings.size());
  descriptorSetEntry.pBindings                       = m_bindings.data();
  descriptorSetEntry.flags                           = flags;

  VkDescriptorSetLayout descriptorSetLayout;
  result = vkCreateDescriptorSetLayout(device, &descriptorSetEntry, pAllocator, &descriptorSetLayout);
  assert(result == VK_SUCCESS);

  return descriptorSetLayout;
}

VkDescriptorPool DescriptorSetReflection::createPool(VkDevice                     device,
                                                     uint32_t                     maxSets /*= 1*/,
                                                     const VkAllocationCallbacks* pAllocator /*= nullptr*/) const
{
  VkResult result;

  // setup poolsizes for each descriptorType
  std::vector<VkDescriptorPoolSize> poolSizes;
  for(auto it = m_bindings.cbegin(); it != m_bindings.cend(); ++it)
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

  VkDescriptorPool           descrPool;
  VkDescriptorPoolCreateInfo descrPoolInfo = {};
  descrPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descrPoolInfo.pNext                      = nullptr;
  descrPoolInfo.maxSets                    = maxSets;
  descrPoolInfo.poolSizeCount              = uint32_t(poolSizes.size());
  descrPoolInfo.pPoolSizes                 = poolSizes.data();

  // scene pool
  result = vkCreateDescriptorPool(device, &descrPoolInfo, pAllocator, &descrPool);
  assert(result == VK_SUCCESS);
  return descrPool;
}

VkDescriptorType DescriptorSetReflection::getType(uint32_t binding) const
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

uint32_t DescriptorSetReflection::getCount(uint32_t binding) const
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

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet dstSet, uint32_t dstBinding) const
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
      return writeSet;
    }
  }
  assert(0 && "binding not found");
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement) const
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

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

  writeSet.pImageInfo = pImageInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

  writeSet.pBufferInfo = pBufferInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);

  writeSet.pTexelBufferView = pTexelBufferView;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const void* pNext) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM);

  writeSet.pNext = pNext;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet                                    dstSet,
                                                       uint32_t                                           dstBinding,
                                                       const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);

  writeSet.pNext = pAccel;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet                                  dstSet,
                                                       uint32_t                                         dstBinding,
                                                       const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT);

  writeSet.pNext = pInline;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet              dstSet,
                                                              uint32_t                     dstBinding,
                                                              uint32_t                     arrayElement,
                                                              const VkDescriptorImageInfo* pImageInfo) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

  writeSet.pImageInfo = pImageInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet               dstSet,
                                                              uint32_t                      dstBinding,
                                                              uint32_t                      arrayElement,
                                                              const VkDescriptorBufferInfo* pBufferInfo) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
         || writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

  writeSet.pBufferInfo = pBufferInfo;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet     dstSet,
                                                              uint32_t            dstBinding,
                                                              uint32_t            arrayElement,
                                                              const VkBufferView* pTexelBufferView) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);

  writeSet.pTexelBufferView = pTexelBufferView;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet dstSet,
                                                              uint32_t        dstBinding,
                                                              uint32_t        arrayElement,
                                                              const void*     pNext) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM);

  writeSet.pNext = pNext;
  return writeSet;
}


VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet dstSet,
                                                              uint32_t        dstBinding,
                                                              uint32_t        arrayElement,
                                                              const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);

  writeSet.pNext = pAccel;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet dstSet,
                                                              uint32_t        dstBinding,
                                                              uint32_t        arrayElement,
                                                              const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT);

  writeSet.pNext = pInline;
  return writeSet;
}


//////////////////////////////////////////////////////////////////////////

static void s_test()
{
  TDescriptorSetContainer<1,1> test;
  test.init(0);
  test.deinit();
}

}  // namespace nvvk
