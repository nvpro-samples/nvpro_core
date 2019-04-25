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

#include "descriptorsetutils_vk.hpp"

namespace nvvk {

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
  std::vector<VkDescriptorSetLayout>  layouts(count, layout);

  VkResult                    result;
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool              = pool;
  allocInfo.descriptorSetCount          = count;
  allocInfo.pSetLayouts                 = layouts.data();

  result = vkAllocateDescriptorSets(device, &allocInfo, sets.data());
  assert(result == VK_SUCCESS);
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

VkDescriptorPool DescriptorSetReflection::createPool(VkDevice device, uint32_t maxSets /*= 1*/, const VkAllocationCallbacks* pAllocator /*= nullptr*/) const
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
  writeSet.descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
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
  writeSet.descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
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
  assert(writeSet.descriptorType != VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);

  writeSet.pNext = pAccel;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWrite(VkDescriptorSet                                  dstSet,
                                                       uint32_t                                         dstBinding,
                                                       const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  VkWriteDescriptorSet writeSet = getWrite(dstSet, dstBinding);
  assert(writeSet.descriptorType != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT);

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
  assert(writeSet.descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM);

  writeSet.pNext = pNext;
  return writeSet;
}


VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet dstSet,
                                                              uint32_t        dstBinding,
                                                              uint32_t        arrayElement,
                                                              const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType != VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV);

  writeSet.pNext = pAccel;
  return writeSet;
}

VkWriteDescriptorSet DescriptorSetReflection::getWriteElement(VkDescriptorSet dstSet,
                                                              uint32_t        dstBinding,
                                                              uint32_t        arrayElement,
                                                              const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const
{
  VkWriteDescriptorSet writeSet = getWriteElement(dstSet, dstBinding, arrayElement);
  assert(writeSet.descriptorType != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT);

  writeSet.pNext = pInline;
  return writeSet;
}

//////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Bind an buffer
void DescriptorSetUpdater::bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkDescriptorBufferInfo>& bufferInfo)
{
  m_buffers.bind(set, binding, 0, m_reflection.getType(binding), bufferInfo);
}

//--------------------------------------------------------------------------------------------------
// Bind an image
void DescriptorSetUpdater::bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfo)
{
  m_images.bind(set, binding, 0, m_reflection.getType(binding), imageInfo);
}

//--------------------------------------------------------------------------------------------------
// Bind an acceleration structure
void DescriptorSetUpdater::bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkWriteDescriptorSetAccelerationStructureNV>& accelInfo)
{
  m_accelerationStructures.bind(set, binding, 0, m_reflection.getType(binding), accelInfo);
}
//--------------------------------------------------------------------------------------------------
// Bind a bufferview
void DescriptorSetUpdater::bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkBufferView>& texelBufferView)
{
  m_texelBuffers.bind(set, binding, 0, m_reflection.getType(binding), texelBufferView);
}

//--------------------------------------------------------------------------------------------------
// Actually write the binding info into the descriptor set
void DescriptorSetUpdater::updateSetContents(VkDevice device, VkDescriptorSet set)
{
  // For each resource type, set the actual pointers in the VkWriteDescriptorSet structures, and
  // write the resulting structures into the descriptor set
  m_buffers.setPointers();
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_buffers.writeDesc.size()), m_buffers.writeDesc.data(), 0, nullptr);

  m_images.setPointers();
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_images.writeDesc.size()), m_images.writeDesc.data(), 0, nullptr);

  m_accelerationStructures.setPointers();
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_accelerationStructures.writeDesc.size()),
                         m_accelerationStructures.writeDesc.data(), 0, nullptr);
}

}  // namespace nvvk
