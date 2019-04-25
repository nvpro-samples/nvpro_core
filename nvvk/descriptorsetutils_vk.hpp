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

namespace nvvk {

/// Generate a descriptor set from the pool and layout
VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);

/// Generate many descriptor sets from the pool and layout
void allocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, uint32_t count, std::vector<VkDescriptorSet>& sets);

/// Helper class generating consistent descriptor pools, layouts and sets
class DescriptorSetReflection
{
public:
  /// Add a binding to the descriptor set
  void addBinding(uint32_t binding,       /// Slot to which the descriptor will be bound, corresponding to the layout
                                          /// binding index in the shader
    VkDescriptorType   type,              /// Type of the bound descriptor(s)
    uint32_t           count,             /// Number of descriptors
    VkShaderStageFlags stageFlags,        /// Shader stages at which the bound resources will be available
    const VkSampler*   pImmutableSampler = nullptr  /// Corresponding sampler, in case of textures
  );

  /// Add a binding to the descriptor set
  void addBinding(VkDescriptorSetLayoutBinding layoutBinding);

  void setBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

  VkDescriptorType getType(uint32_t binding) const;
  uint32_t         getCount(uint32_t binding) const;

  /// Once the bindings have been added, this generates the descriptor layout corresponding to the
  /// bound resources
  VkDescriptorSetLayout createLayout(VkDevice device, VkDescriptorSetLayoutCreateFlags flags = 0, const VkAllocationCallbacks* pAllocator = nullptr) const;
  
  /// Once the bindings have been added, this generates the descriptor pool with enough space to
  /// handle all the bound resources and allocate up to maxSets descriptor sets
  VkDescriptorPool createPool(VkDevice device, uint32_t maxSets = 1, const VkAllocationCallbacks* pAllocator = nullptr) const;

  // if dstBinding is an array assumes all are provided
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkWriteDescriptorSetAccelerationStructureNV * pAccel) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const VkWriteDescriptorSetInlineUniformBlockEXT * pInline) const;
  VkWriteDescriptorSet getWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const void* pNext) const;

  // single element for array
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorImageInfo* pImageInfo) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorBufferInfo* pBufferInfo) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkWriteDescriptorSetAccelerationStructureNV * pAccel) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkWriteDescriptorSetInlineUniformBlockEXT * pInline) const;
  VkWriteDescriptorSet getWriteElement(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const void* pNext) const;
  
private:
  std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

//////////////////////////////////////////////////////////////////////////

class DescriptorSetUpdater
{
public:
  /// Utility class that temporarily stores all binding information
  /// until updateSetContents is called.

  DescriptorSetUpdater(const DescriptorSetReflection& reflection) : m_reflection(reflection) {}

  /// Store the information to write into one descriptor set entry: the number of descriptors of the
  /// entry, and where in the descriptor the buffer information should be written
  template <typename T  /// Type of the descriptor info, such as VkDescriptorBufferInfo
    ,
    uint32_t offset  /// Offset in the VkWriteDescriptorSet structure where the descriptor
                     /// info should be written, which depends on the type of the info. For
                     /// example, a VkDescriptorBufferInfo needs to be written in the
                     /// pBufferInfo member, while VkDescriptorImageInfo has to be written in
                     /// pImageInfo
  >
  struct WriteInfo
  {
    /// Write descriptors
    std::vector<VkWriteDescriptorSet> writeDesc;
    /// Contents to write in one of the info members of the descriptor
    std::vector<std::vector<T>> contents;

    /// Since the VkWriteDescriptorSet structure requires pointers to the info descriptors, and we
    /// use std::vector to store those, the pointers can be set only when we are finished adding
    /// data in the vectors. The SetPointers then writes the info descriptor at the proper offset in
    /// the VkWriteDescriptorSet structure
    void setPointers()
    {
      for (size_t i = 0; i < writeDesc.size(); i++)
      {
        T** dest = reinterpret_cast<T**>(reinterpret_cast<uint8_t*>(&writeDesc[i]) + offset);
        *dest = contents[i].data();
      }
    }
    /// Bind a vector of info descriptors to a slot in the descriptor set
    void bind(VkDescriptorSet       set,      /// Target descriptor set
      uint32_t              binding,  /// Slot in the descriptor set the infos will be bound to
      uint32_t              arrayElement,
      VkDescriptorType      type,     /// Type of the descriptor
      const std::vector<T>& info      /// Descriptor infos to bind
    )
    {
      // Initialize the descriptor write, keeping all the resource pointers to NULL since they will
      // be set by SetPointers once all resources have been bound
      VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
      descriptorWrite.dstSet = set;
      descriptorWrite.dstBinding = binding;
      descriptorWrite.dstArrayElement = arrayElement;
      descriptorWrite.descriptorType = type;
      descriptorWrite.descriptorCount = static_cast<uint32_t>(info.size());
      descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
      descriptorWrite.pImageInfo = VK_NULL_HANDLE;
      descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
      descriptorWrite.pNext = VK_NULL_HANDLE;

      // If the binding point had already been used in a Bind call, replace the binding info
      // Linear search, not so great - hopefully not too many binding points
      for (size_t i = 0; i < writeDesc.size(); i++)
      {
        if (writeDesc[i].dstBinding == binding)
        {
          writeDesc[i] = descriptorWrite;
          contents[i] = info;
          return;
        }
      }
      // Add the write descriptor and resource info for later actual binding
      writeDesc.push_back(descriptorWrite);
      contents.push_back(info);
    }
  };

  /// Bind a buffer
  void bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkDescriptorBufferInfo>& bufferInfo);

  /// Bind an image
  void bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfo);

  /// Bind a bufferview
  void bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkBufferView>& texelBufferView);

  /// Bind an acceleration structure
  void bind(VkDescriptorSet set, uint32_t binding, const std::vector<VkWriteDescriptorSetAccelerationStructureNV>& accelInfo);

  /// Actually write the binding info into the descriptor set
  void updateSetContents(VkDevice device, VkDescriptorSet set);

private:
  /// Association of the binding slot index with the binding information
  const DescriptorSetReflection& m_reflection;

  /// Buffer binding requests. Buffer descriptor infos are written into the pBufferInfo member of
  /// the VkWriteDescriptorSet structure
  WriteInfo<VkDescriptorBufferInfo, offsetof(VkWriteDescriptorSet, pBufferInfo)> m_buffers;
  /// Image binding requests. Image descriptor infos are written into the pImageInfo member of
  /// the VkWriteDescriptorSet structure
  WriteInfo<VkDescriptorImageInfo, offsetof(VkWriteDescriptorSet, pImageInfo)> m_images;
  /// TexelBuffer binding requests. BufferView descriptr infos are written into the pTexelBufferView member of
  /// the VkWriteDescriptorSet structure
  WriteInfo<VkBufferView, offsetof(VkWriteDescriptorSet, pTexelBufferView)> m_texelBuffers;
  /// Acceleration structure binding requests. Since this is using an non-core extension, AS
  /// descriptor infos are written into the pNext member of the VkWriteDescriptorSet structure
  WriteInfo<VkWriteDescriptorSetAccelerationStructureNV, offsetof(VkWriteDescriptorSet, pNext)> m_accelerationStructures;

};



} // namespace nvvk