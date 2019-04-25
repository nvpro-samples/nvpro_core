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

#include "descriptorsetcontainer_vk.hpp"

namespace nvvk {

void DescriptorSetContainer::init(VkDevice device, const VkAllocationCallbacks* pAllocator /*= nullptr*/)
{
  m_device    = device;
  m_allocator = pAllocator;
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

void DescriptorSetContainer::initLayout(VkDescriptorSetLayoutCreateFlags flags /*= 0*/)
{
  m_layout = m_reflection.createLayout(m_device, flags, m_allocator);
}

void DescriptorSetContainer::initPool(uint32_t numAllocatedSets)
{
  m_pool = m_reflection.createPool(m_device, numAllocatedSets, m_allocator);
  allocateDescriptorSets(m_device, m_pool, m_layout, numAllocatedSets, m_descriptorSets);
}

void DescriptorSetContainer::initPipeLayout(uint32_t                    numRanges /*= 0*/,
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

  result = vkCreatePipelineLayout(m_device, &layoutCreateInfo, m_allocator, &m_pipelineLayout);
  assert(result == VK_SUCCESS);
}

void DescriptorSetContainer::deinitPool()
{
  if (!m_descriptorSets.empty())
  {
    m_descriptorSets.clear();
  }

  if(m_pool)
  {
    vkDestroyDescriptorPool(m_device, m_pool, m_allocator);
    m_pool = VK_NULL_HANDLE;
  }
}

void DescriptorSetContainer::deinitLayout()
{
  if(m_pipelineLayout)
  {
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, m_allocator);
    m_pipelineLayout = VK_NULL_HANDLE;
  }

  if(m_layout)
  {
    vkDestroyDescriptorSetLayout(m_device, m_layout, m_allocator);
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

static void s_test()
{
  TDescriptorSetContainer<1,1> test;
  test.init(0,0);
  test.deinit();
}


}  // namespace nvvk
