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
#include <vulkan/vulkan_core.h>

namespace nvvk {

/**
  # functions in nvvk

  - createDescriptorSetLayout : wrappers for vkCreateDescriptorSetLayout
  - createDescriptorPool : wrappers for vkCreateDescriptorPool
  - allocateDescriptorSet : allocates a single VkDescriptorSet
  - allocateDescriptorSets : allocates multiple VkDescriptorSets

*/
VkDescriptorSetLayout createDescriptorSetLayout(VkDevice                            device,
                                                size_t                              numBindings,
                                                const VkDescriptorSetLayoutBinding* bindings,
                                                VkDescriptorSetLayoutCreateFlags    flags = 0);

NV_INLINE VkDescriptorSetLayout createDescriptorSetLayout(VkDevice                                         device,
                                                          const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                                                          VkDescriptorSetLayoutCreateFlags                 flags = 0)
{
  return createDescriptorSetLayout(device, bindings.size(), bindings.data(), flags);
}

VkDescriptorPool createDescriptorPool(VkDevice device, size_t poolSizeCount, const VkDescriptorPoolSize* poolSizes, uint32_t maxSets);

NV_INLINE VkDescriptorPool createDescriptorPool(VkDevice                                 device,
                                                const std::vector<VkDescriptorPoolSize>& poolSizes,
                                                VkDescriptorSetLayoutCreateFlags         maxSets)
{
  return createDescriptorPool(device, poolSizes.size(), poolSizes.data(), maxSets);
}

/// Generate a descriptor set from the pool and layout
VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);

/// Generate many descriptor sets from the pool and layout
void allocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, uint32_t count, std::vector<VkDescriptorSet>& sets);

/////////////////////////////////////////////////////////////////////////////
/**
# class nvvk::DescriptorSetReflection

Helper class that keeps a collection of `VkDescriptorSetLayoutBinding` for a single
`VkDescriptorSetLayout`. Provides helper functions to create `VkDescriptorSetLayout`
as well as `VkDescriptorPool` based on this information, as well as utilities
to fill the `VkWriteDescriptorSet` structure with binding information stored
within the class.

Example :
~~~C++
DescriptorSetReflection refl;

refl.addBinding( VIEW_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
refl.addBinding(XFORM_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

VkDescriptorSetLayout layout = refl.createLayout(device);

// let's create a pool with 2 sets
VkDescriptorPool      pool   = refl.createPool(device, 2);

std::vector<VkWriteDescriptorSet> updates;

fill them
updates.push_back(refl.getWrite(0, VIEW_BINDING, &view0BufferInfo));
updates.push_back(refl.getWrite(1, VIEW_BINDING, &view1BufferInfo));
updates.push_back(refl.getWrite(0, XFORM_BINDING, &xform0BufferInfo));
updates.push_back(refl.getWrite(1, XFORM_BINDING, &xform1BufferInfo));

vkUpdateDescriptorSets(device, updates.size(), updates.data(), 0, nullptr);
~~~
*/

class DescriptorSetReflection
{
public:
  /// Add a binding to the descriptor set
  void addBinding(uint32_t binding,          /// Slot to which the descriptor will be bound, corresponding to the layout
                                             /// binding index in the shader
                  VkDescriptorType   type,   /// Type of the bound descriptor(s)
                  uint32_t           count,  /// Number of descriptors
                  VkShaderStageFlags stageFlags,  /// Shader stages at which the bound resources will be available
                  const VkSampler*   pImmutableSampler = nullptr  /// Corresponding sampler, in case of textures
  );

  /// Add a binding to the descriptor set
  void addBinding(VkDescriptorSetLayoutBinding layoutBinding);

  void setBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

  VkDescriptorType getType(uint32_t binding) const;
  uint32_t         getCount(uint32_t binding) const;

  /// Once the bindings have been added, this generates the descriptor layout corresponding to the
  /// bound resources
  VkDescriptorSetLayout createLayout(VkDevice                         device,
                                     VkDescriptorSetLayoutCreateFlags flags      = 0,
                                     const VkAllocationCallbacks*     pAllocator = nullptr) const;

  /// Once the bindings have been added, this generates the descriptor pool with enough space to
  /// handle all the bound resources and allocate up to maxSets descriptor sets
  VkDescriptorPool createPool(VkDevice device, uint32_t maxSets = 1, const VkAllocationCallbacks* pAllocator = nullptr) const;

  // if dstBinding is an array assumes all are provided
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const void* pNext) const;

  // single element for array
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet              dstSet,
                                       uint32_t                     dstBinding,
                                       uint32_t                     arrayElement,
                                       const VkDescriptorImageInfo* pImageInfo) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet               dstSet,
                                       uint32_t                      dstBinding,
                                       uint32_t                      arrayElement,
                                       const VkDescriptorBufferInfo* pBufferInfo) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet                                    dstSet,
                                       uint32_t                                           dstBinding,
                                       uint32_t                                           arrayElement,
                                       const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet                                  dstSet,
                                       uint32_t                                         dstBinding,
                                       uint32_t                                         arrayElement,
                                       const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const void* pNext) const;

private:
  std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

/////////////////////////////////////////////////////////////
/**
# class nvvk::DescriptorSetContainer

Container class that stores allocated DescriptorSets
as well as reflection, layout and pool

Example:
~~~ C++
    container.init(device, allocator);

    // setup dset layouts
    container.addBinding(0, UBO...)
    container.addBinding(1, SSBO...)
    container.initLayout();

    // allocate descriptorsets
    container.initPool(17);

    // update descriptorsets
    writeUpdates.push_back( container.getWrite(0, 0, ..) );
    writeUpdates.push_back( container.getWrite(0, 1, ..) );
    writeUpdates.push_back( container.getWrite(1, 0, ..) );
    writeUpdates.push_back( container.getWrite(1, 1, ..) );
    writeUpdates.push_back( container.getWrite(2, 0, ..) );
    writeUpdates.push_back( container.getWrite(2, 1, ..) );
    ...

    // at render time

    vkCmdBindDescriptorSets(cmd, GRAPHICS, pipeLayout, 1, 1, container.at(7).getSets());
~~~

*/
class DescriptorSetContainer
{

protected:
  VkDevice                     m_device         = VK_NULL_HANDLE;
  VkDescriptorSetLayout        m_layout         = VK_NULL_HANDLE;
  VkDescriptorPool             m_pool           = VK_NULL_HANDLE;
  VkPipelineLayout             m_pipelineLayout = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_descriptorSets = {};
  DescriptorSetReflection      m_reflection     = {};

  //////////////////////////////////////////////////////////////////////////
public:
  DescriptorSetContainer() {}
  DescriptorSetContainer(VkDevice device)
  {
    init(device);
  }
  void init(VkDevice device);

  void setBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
  void addBinding(VkDescriptorSetLayoutBinding layoutBinding);
  void addBinding(uint32_t           binding,
                  VkDescriptorType   descriptorType,
                  uint32_t           descriptorCount,
                  VkShaderStageFlags stageFlags,
                  const VkSampler*   pImmutableSamplers = nullptr);


  VkDescriptorSetLayout initLayout(VkDescriptorSetLayoutCreateFlags flags = 0);
  /// inits pool and immediately allocates all numSets-many DescriptorSets
  VkDescriptorPool initPool(uint32_t numAllocatedSets);

  /// optionally generates a pipelinelayout for the descriptorsetlayout
  VkPipelineLayout initPipeLayout(uint32_t                    numRanges = 0,
                                  const VkPushConstantRange*  ranges    = nullptr,
                                  VkPipelineLayoutCreateFlags flags     = 0);

  void deinitPool();
  void deinitLayout();
  void deinit();

  //////////////////////////////////////////////////////////////////////////

  VkDescriptorSet        getSet(uint32_t dstSetIdx = 0) const { return m_descriptorSets[dstSetIdx]; }
  const VkDescriptorSet* getSets(uint32_t dstSetIdx = 0) const { return m_descriptorSets.data() + dstSetIdx; }
  uint32_t               getSetsCount() const { return (uint32_t)m_descriptorSets.size(); }

  VkDescriptorSetLayout         getLayout() const { return m_layout; }
  VkPipelineLayout              getPipeLayout() const { return m_pipelineLayout; }
  const DescriptorSetReflection getRef() const { return m_reflection; }

  VkDevice                       getDevice() const { return m_device; }
  const VkAllocationCallbacks*   getAllocationCallbacks() const { return nullptr; }
  const DescriptorSetReflection& getDescriptorSetReflection() const { return m_reflection; }
                                 operator DescriptorSetReflection&() { return m_reflection; }

  //////////////////////////////////////////////////////////////////////////

  // if dstBinding is an array assumes all are provided (pInfo is array as well)
  VkWriteDescriptorSet getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const;
  VkWriteDescriptorSet getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const;
  VkWriteDescriptorSet getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const;
  VkWriteDescriptorSet getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const;
  VkWriteDescriptorSet getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const;
  VkWriteDescriptorSet getWrite(uint32_t dstSetIdx, uint32_t dstBinding, const void* pNext) const;

  // single element for array
  VkWriteDescriptorSet getWriteElement(uint32_t dstSetIdx, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorImageInfo* pImageInfo) const;
  VkWriteDescriptorSet getWriteElement(uint32_t dstSetIdx, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorBufferInfo* pBufferInfo) const;
  VkWriteDescriptorSet getWriteElement(uint32_t dstSetIdx, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView) const;
  VkWriteDescriptorSet getWriteElement(uint32_t                                           dstSetIdx,
                                       uint32_t                                           dstBinding,
                                       uint32_t                                           arrayElement,
                                       const VkWriteDescriptorSetAccelerationStructureNV* pAccel) const;
  VkWriteDescriptorSet getWriteElement(uint32_t                                         dstSetIdx,
                                       uint32_t                                         dstBinding,
                                       uint32_t                                         arrayElement,
                                       const VkWriteDescriptorSetInlineUniformBlockEXT* pInline) const;
  VkWriteDescriptorSet getWriteElement(uint32_t dstSetIdx, uint32_t dstBinding, uint32_t arrayElement, const void* pNext) const;
};

//////////////////////////////////////////////////////////////////////////
/**
# class nvvk::TDescriptorSetContainer<SETS,PIPES=1>

Templated version of DescriptorSetContainer :

- SETS  - many DescriptorSetContainers
- PIPES - many VkPipelineLayouts

The pipeline layouts are stored separately, the class does
not use the pipeline layouts of the embedded DescriptorSetContainers.

Example :

~~~ C++
Usage, e.g.SETS = 2, PIPES = 2

container.init(device, allocator);

// setup dset layouts
container.at(0).addBinding(0, UBO...)
container.at(0).addBinding(1, SSBO...)
container.at(0).initLayout();
container.at(1).addBinding(0, COMBINED_SAMPLER...)
container.at(1).initLayout();

// pipe 0 uses set 0 alone
container.initPipeLayout(0, 1);
// pipe 1 uses sets 0, 1
container.initPipeLayout(1, 2);

// allocate descriptorsets
container.at(0).initPool(1);
container.at(1).initPool(16);

// update descriptorsets

writeUpdates.push_back(container.at(0).getWrite(0, 0, ..));
writeUpdates.push_back(container.at(0).getWrite(0, 1, ..));
writeUpdates.push_back(container.at(1).getWrite(0, 0, ..));
writeUpdates.push_back(container.at(1).getWrite(1, 0, ..));
writeUpdates.push_back(container.at(1).getWrite(2, 0, ..));
...

// at render time

vkCmdBindDescriptorSets(cmd, GRAPHICS, container.getPipeLayout(0), 0, 1, container.at(0).getSets());
..
vkCmdBindDescriptorSets(cmd, GRAPHICS, container.getPipeLayout(1), 1, 1, container.at(1).getSets(7));
~~~
*/
template <int SETS, int PIPES=1>
class TDescriptorSetContainer
{
public:
  TDescriptorSetContainer() {}
  TDescriptorSetContainer(VkDevice device)
  {
    init(device);
  }
  void init(VkDevice device);
  void deinit();
  void deinitLayouts();
  void deinitPools();

  // pipelayout uses range of m_sets[0.. first null or SETS[
  VkPipelineLayout initPipeLayout(uint32_t                    pipe,
                                  uint32_t                    numRanges = 0,
                                  const VkPushConstantRange*  ranges    = nullptr,
                                  VkPipelineLayoutCreateFlags flags     = 0);

  // pipelayout uses range of m_sets[0..numDsets[
  VkPipelineLayout initPipeLayout(uint32_t                    pipe,
                                  uint32_t                    numDsets,
                                  uint32_t                    numRanges = 0,
                                  const VkPushConstantRange*  ranges    = nullptr,
                                  VkPipelineLayoutCreateFlags flags     = 0);

  DescriptorSetContainer&               at(uint32_t set) { return m_sets[set]; }
  const DescriptorSetContainer&         at(uint32_t set) const { return m_sets[set]; }
  DescriptorSetContainer&               operator[](uint32_t set) { return m_sets[set]; }
  const DescriptorSetContainer&         operator[](uint32_t set) const { return m_sets[set]; }
  VkPipelineLayout getPipeLayout(uint32_t pipe = 0) const { return m_pipelayouts[pipe]; }

protected:
  VkPipelineLayout        m_pipelayouts[PIPES] = {};
  DescriptorSetContainer  m_sets[SETS];
};

//////////////////////////////////////////////////////////////////////////

template <int SETS, int PIPES>
VkPipelineLayout TDescriptorSetContainer<SETS, PIPES>::initPipeLayout(uint32_t                   pipe,
                                                                          uint32_t                   numDsets,
                                                                          uint32_t                   numRanges /*= 0*/,
                                                                          const VkPushConstantRange* ranges /*= nullptr*/,
                                                                          VkPipelineLayoutCreateFlags flags /*= 0*/)
{
  VkDevice                     device     = m_sets[0].getDevice();
  const VkAllocationCallbacks* pAllocator = m_sets[0].getAllocationCallbacks();

  VkDescriptorSetLayout setLayouts[SETS];
  for(int d = 0; d < SETS; d++)
  {
    setLayouts[d] = m_sets[d].getLayout();
  }

  VkResult                   result;
  VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layoutCreateInfo.setLayoutCount             = numDsets;
  layoutCreateInfo.pSetLayouts                = setLayouts;
  layoutCreateInfo.pushConstantRangeCount     = numRanges;
  layoutCreateInfo.pPushConstantRanges        = ranges;
  layoutCreateInfo.flags                      = flags;

  result = vkCreatePipelineLayout(device, &layoutCreateInfo, pAllocator, &m_pipelayouts[pipe]);
  assert(result == VK_SUCCESS);
  return m_pipelayouts[pipe];
}

template <int SETS, int PIPES>
VkPipelineLayout TDescriptorSetContainer<SETS, PIPES>::initPipeLayout(uint32_t                   pipe,
                                                                          uint32_t                   numRanges /*= 0*/,
                                                                          const VkPushConstantRange* ranges /*= nullptr*/,
                                                                          VkPipelineLayoutCreateFlags flags /*= 0*/)
{
  VkDevice                     device     = m_sets[0].getDevice();
  const VkAllocationCallbacks* pAllocator = m_sets[0].getAllocationCallbacks();

  VkDescriptorSetLayout setLayouts[SETS];
  int                   used;
  for(used = 0; used < SETS; used++)
  {
    setLayouts[used] = m_sets[used].getLayout();
    if(!setLayouts[used])
      break;
  }

  VkResult                   result;
  VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layoutCreateInfo.setLayoutCount             = uint32_t(used);
  layoutCreateInfo.pSetLayouts                = setLayouts;
  layoutCreateInfo.pushConstantRangeCount     = numRanges;
  layoutCreateInfo.pPushConstantRanges        = ranges;
  layoutCreateInfo.flags                      = flags;

  result = vkCreatePipelineLayout(device, &layoutCreateInfo, pAllocator, &m_pipelayouts[pipe]);
  assert(result == VK_SUCCESS);
  return m_pipelayouts[pipe];
}

template <int SETS, int PIPES>
void TDescriptorSetContainer<SETS, PIPES>::deinitPools()
{
  for(int d = 0; d < SETS; d++)
  {
    m_sets[d].deinitPool();
  }
}

template <int SETS, int PIPES>
void TDescriptorSetContainer<SETS, PIPES>::deinitLayouts()
{
  VkDevice                     device     = m_sets[0].getDevice();
  const VkAllocationCallbacks* pAllocator = m_sets[0].getAllocationCallbacks();

  for(int p = 0; p < PIPES; p++)
  {
    if(m_pipelayouts[p])
    {
      vkDestroyPipelineLayout(device, m_pipelayouts[p], pAllocator);
      m_pipelayouts[p] = VK_NULL_HANDLE;
    }
  }
  for(int d = 0; d < SETS; d++)
  {
    m_sets[d].deinitLayout();
  }
}

template <int SETS, int PIPES>
void TDescriptorSetContainer<SETS, PIPES>::deinit()
{
  deinitPools();
  deinitLayouts();
}

template <int SETS, int PIPES>
void TDescriptorSetContainer<SETS, PIPES>::init(VkDevice device)
{
  for(int d = 0; d < SETS; d++)
  {
    m_sets[d].init(device);
  }
}


}  // namespace nvvk
