/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#include "base_vk.hpp"

#include <algorithm>

namespace nv_helpers_vk {

  VkBufferCreateInfo Makers::makeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
  {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.flags = flags;

    return bufferInfo;
  }

  VkBufferViewCreateInfo Makers::makeBufferViewCreateInfo(VkDescriptorBufferInfo &descrInfo, VkFormat fmt, VkBufferViewCreateFlags flags)
  {
    VkBufferViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    createInfo.buffer = descrInfo.buffer;
    createInfo.offset = descrInfo.offset;
    createInfo.range = descrInfo.range;
    createInfo.flags = flags;
    createInfo.format = fmt;

    return createInfo;
  }

  VkDescriptorBufferInfo Makers::makeDescriptorBufferInfo(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset)
  {
    VkDescriptorBufferInfo info;
    info.buffer = buffer;
    info.offset = offset;
    info.range = size;

    assert(size);

    return info;
  }

  VkDescriptorSetLayoutBinding Makers::makeDescriptorSetLayoutBinding(VkDescriptorType type, VkPipelineStageFlags flags, uint32_t bindingSlot, const VkSampler * pSamplers, uint32_t count)
  {
    VkDescriptorSetLayoutBinding binding;
    binding.descriptorType = type;
    binding.descriptorCount = count;
    binding.pImmutableSamplers = pSamplers;
    binding.stageFlags = flags;
    binding.binding = bindingSlot;
    return binding;
  }

  uint32_t Makers::makeAccessMaskPipelineStageFlags(uint32_t accessMask)
  {
    static const uint32_t accessPipes[] = {
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
      VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
      VK_ACCESS_INDEX_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_UNIFORM_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_ACCESS_TRANSFER_READ_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_HOST_READ_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,
      VK_ACCESS_HOST_WRITE_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      0,
      VK_ACCESS_MEMORY_WRITE_BIT,
      0,
      VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX,
      VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX,
      VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX,
      VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX,
    };

    if (!accessMask) {
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    for (uint32_t i = 0; i < NV_ARRAY_SIZE(accessPipes); i += 2) {
      if (accessPipes[i] & accessMask) {
        return accessPipes[i + 1];
      }
    }
    assert(0);

    return 0;
  }
  
  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorBufferInfo* pBufferInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.pBufferInfo = pBufferInfo;
        assert(
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const VkDescriptorImageInfo* pImageInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.pImageInfo = pImageInfo;
        assert(
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const VkBufferView* pTexelBufferView)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.pTexelBufferView = pTexelBufferView;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, const void* pNext)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.pNext = pNext;
        writeSet.descriptorCount = bindings[i].descriptorCount;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorBufferInfo* pBufferInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstArrayElement = arrayElement;
        writeSet.dstSet = dstSet;
        writeSet.pBufferInfo = pBufferInfo;
        assert(
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkDescriptorImageInfo* pImageInfo)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.dstArrayElement = arrayElement;
        writeSet.pImageInfo = pImageInfo;
        assert(
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
          writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const VkBufferView* pTexelBufferView)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.dstArrayElement = arrayElement;
        writeSet.pTexelBufferView = pTexelBufferView;
        assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  VkWriteDescriptorSet Makers::makeWriteDescriptorSet(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t arrayElement, const void* pNext)
  {
    VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    for (size_t i = 0; i < numBindings; i++) {
      if (bindings[i].binding == dstBinding) {
        writeSet.pNext = pNext;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = bindings[i].descriptorType;
        writeSet.dstBinding = dstBinding;
        writeSet.dstSet = dstSet;
        writeSet.dstArrayElement = arrayElement;
        return writeSet;
      }
    }
    assert(0);
    return writeSet;
  }

  uint32_t PhysicalDeviceMemoryProperties_getMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties, const VkMemoryRequirements &memReqs, VkFlags memProps)
  {
    // Find an available memory type that satisfies the requested properties.
    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex) {
      if ((memReqs.memoryTypeBits & (1 << memoryTypeIndex)) &&
        (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps)
      {
        return memoryTypeIndex;
      }
    }
    return ~0;
  }

  bool PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties, const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfo)
  {
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.pNext = nullptr;

    if (!memReqs.size) {
      memInfo.allocationSize = 0;
      memInfo.memoryTypeIndex = ~0;
      return true;
    }

    uint32_t memoryTypeIndex = PhysicalDeviceMemoryProperties_getMemoryTypeIndex(memoryProperties, memReqs, memProps);
    if (memoryTypeIndex == ~0) {
      return false;
    }

    memInfo.allocationSize = memReqs.size;
    memInfo.memoryTypeIndex = memoryTypeIndex;

    return true;
  }

  bool PhysicalDeviceMemoryProperties_appendMemoryAllocationInfo(const VkPhysicalDeviceMemoryProperties& memoryProperties, const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfoAppended, VkDeviceSize &offset)
  {
    VkMemoryAllocateInfo memInfo;
    if (!PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfo)) {
      return false;
    }
    if (memInfoAppended.allocationSize == 0) {
      memInfoAppended = memInfo;
      offset = 0;
      return true;
    }
    else if (memInfoAppended.memoryTypeIndex != memInfo.memoryTypeIndex) {
      return false;
    }
    else {
      offset = (memInfoAppended.allocationSize + memReqs.alignment - 1) & ~(memReqs.alignment - 1);
      memInfoAppended.allocationSize = offset + memInfo.allocationSize;
      return true;
    }
  }

  //////////////////////////////////////////////////////////////////////////


  void PhysicalInfo::init(VkPhysicalDevice physicalDeviceIn, uint32_t apiMajor, uint32_t apiMinor)
  {
    physicalDevice = physicalDeviceIn;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    queueProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueProperties.data());

    memset(&extFeatures, 0, sizeof(extFeatures));
    memset(&extProperties, 0, sizeof(extProperties));
    
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    extFeatures.multiview.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
    extFeatures.t16BitStorage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
    extFeatures.samplerYcbcrConversion.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
    extFeatures.protectedMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
    extFeatures.drawParameters.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
    extFeatures.variablePointers.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES;

    features2.pNext = &extFeatures.multiview;
    extFeatures.multiview.pNext = &extFeatures.t16BitStorage;
    extFeatures.t16BitStorage.pNext = &extFeatures.samplerYcbcrConversion;
    extFeatures.samplerYcbcrConversion.pNext = &extFeatures.protectedMemory;
    extFeatures.protectedMemory.pNext = &extFeatures.drawParameters;
    extFeatures.drawParameters.pNext = &extFeatures.variablePointers;
    extFeatures.variablePointers.pNext = nullptr;

    VkPhysicalDeviceProperties2           properties2;

    properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    extProperties.maintenance3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
    extProperties.deviceID.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    extProperties.multiview.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
    extProperties.protectedMemory.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
    extProperties.pointClipping.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
    extProperties.subgroup.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

    properties2.pNext = &extProperties.maintenance3;
    extProperties.maintenance3.pNext = &extProperties.subgroup;
    extProperties.deviceID.pNext = &extProperties.multiview;
    extProperties.multiview.pNext = &extProperties.protectedMemory;
    extProperties.protectedMemory.pNext = &extProperties.pointClipping;
    extProperties.pointClipping.pNext = &extProperties.subgroup;
    extProperties.subgroup.pNext = nullptr;

    if (apiMajor == 1 && apiMinor > 0) {
      vkGetPhysicalDeviceFeatures2(physicalDeviceIn, &features2);
      vkGetPhysicalDeviceProperties2(physicalDeviceIn, &properties2);
    }
    else {
      vkGetPhysicalDeviceProperties(physicalDevice, &properties2.properties);
      vkGetPhysicalDeviceFeatures(physicalDevice, &features2.features);
    }

    properties = properties2.properties;
  }
  
  bool PhysicalInfo::getOptimalDepthStencilFormat(VkFormat &depthStencilFormat) const
  {
    VkFormat depthStencilFormats[] = {
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
    };

    for (size_t i = 0; i < NV_ARRAY_SIZE(depthStencilFormats); i++)
    {
      VkFormat format = depthStencilFormats[i];
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
      // Format must support depth stencil attachment for optimal tiling
      if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      {
        depthStencilFormat = format;
        return true;
      }
    }

    return false;
  }

  uint32_t PhysicalInfo::getExclusiveQueueFamily(VkQueueFlagBits bits) const
  {
    for (uint32_t i = 0; i < uint32_t(queueProperties.size()); i++) {
      if ((queueProperties[i].queueFlags & bits) == bits && !(queueProperties[i].queueFlags & ~bits)) {
        return i;
      }
    }
    return VK_QUEUE_FAMILY_IGNORED;
  }

  uint32_t PhysicalInfo::getQueueFamily(VkQueueFlags bits /*= VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT*/) const
  {
    for (uint32_t i = 0; i < uint32_t(queueProperties.size()); i++) {
      if ((queueProperties[i].queueFlags & bits) == bits) {
        return i;
      }
    }
    return VK_QUEUE_FAMILY_IGNORED;
  }

  uint32_t PhysicalInfo::getPresentQueueFamily(VkSurfaceKHR surface, VkQueueFlags bits/*=VK_QUEUE_GRAPHICS_BIT*/) const
  {
    for (uint32_t i = 0; i < uint32_t(queueProperties.size()); i++) {
      VkBool32 supportsPresent;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);

      if (supportsPresent && ((queueProperties[i].queueFlags & bits) == bits)) {
        return i;
      }
    }
    return VK_QUEUE_FAMILY_IGNORED;
  }
  
  bool PhysicalInfo::getMemoryAllocationInfo(const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfo) const
  {
    return PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfo);
  }

  bool PhysicalInfo::appendMemoryAllocationInfo(const VkMemoryRequirements &memReqs, VkFlags memProps, VkMemoryAllocateInfo &memInfoAppended, VkDeviceSize &offset) const
  {
    return PhysicalDeviceMemoryProperties_appendMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfoAppended, offset);
  }

  //////////////////////////////////////////////////////////////////////////

  void Submission::setQueue(VkQueue queue)
  {
    assert(m_waits.empty() && m_waitFlags.empty() && m_signals.empty() && m_commands.empty());
    m_queue = queue;
  }

  uint32_t Submission::getCommandBufferCount() const
  {
    return uint32_t(m_commands.size());
  }

  void Submission::enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers)
  {
    m_commands.reserve(m_commands.size() + num);
    for (uint32_t i = 0; i < num; i++) {
      m_commands.push_back(cmdbuffers[i]);
    }
  }

  void Submission::enqueue(VkCommandBuffer cmdbuffer)
  {
    m_commands.push_back(cmdbuffer);
  }

  void Submission::enqueueAt(uint32_t pos, VkCommandBuffer cmdbuffer)
  {
    m_commands.insert(m_commands.begin() + pos, cmdbuffer);
  }

  void Submission::enqueueSignal(VkSemaphore sem)
  {
    m_signals.push_back(sem);
  }

  void Submission::enqueueWait(VkSemaphore sem, VkPipelineStageFlags flag)
  {
    m_waits.push_back(sem);
    m_waitFlags.push_back(flag);
  }

  VkResult Submission::execute(VkFence fence /*= nullptr*/, uint32_t deviceMask)
  {
    VkResult res = VK_SUCCESS;

    if (m_queue && (fence || !m_commands.empty() || !m_signals.empty() || !m_waits.empty())) {
      VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
      submitInfo.commandBufferCount = uint32_t(m_commands.size());
      submitInfo.signalSemaphoreCount = uint32_t(m_signals.size());
      submitInfo.waitSemaphoreCount = uint32_t(m_waits.size());

      submitInfo.pCommandBuffers = m_commands.data();
      submitInfo.pSignalSemaphores = m_signals.data();
      submitInfo.pWaitSemaphores = m_waits.data();
      submitInfo.pWaitDstStageMask = m_waitFlags.data();

      std::vector<uint32_t> deviceMasks;
      std::vector<uint32_t> deviceIndices;

      VkDeviceGroupSubmitInfo deviceGroupInfo = { VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO };

      if (deviceMask != 0)
      {
        // Allocate an array big enough to hold the mask for all three parameters
        deviceMasks.resize(m_commands.size(), deviceMask);
        deviceIndices.resize(std::max(m_signals.size(), m_waits.size()), 0); // Only perform semaphore actions on device zero

        submitInfo.pNext = &deviceGroupInfo;
        deviceGroupInfo.commandBufferCount = submitInfo.commandBufferCount;
        deviceGroupInfo.pCommandBufferDeviceMasks = deviceMasks.data();
        deviceGroupInfo.signalSemaphoreCount = submitInfo.signalSemaphoreCount;
        deviceGroupInfo.pSignalSemaphoreDeviceIndices = deviceIndices.data();
        deviceGroupInfo.waitSemaphoreCount = submitInfo.waitSemaphoreCount;
        deviceGroupInfo.pWaitSemaphoreDeviceIndices = deviceIndices.data();
      }

      res = vkQueueSubmit(m_queue, 1, &submitInfo, fence);

      m_commands.clear();
      m_waits.clear();
      m_waitFlags.clear();
      m_signals.clear();
    }

    return res;
  }

  //////////////////////////////////////////////////////////////////////////

  void DeviceUtils::createDescriptorPoolAndSets(uint32_t maxSets, size_t poolSizeCount, const VkDescriptorPoolSize* poolSizes, VkDescriptorSetLayout layout, VkDescriptorPool& pool, VkDescriptorSet* sets) const
  {
    VkResult result;

    VkDescriptorPool descrPool;
    VkDescriptorPoolCreateInfo descrPoolInfo = {};
    descrPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descrPoolInfo.pNext = nullptr;
    descrPoolInfo.maxSets = maxSets;
    descrPoolInfo.poolSizeCount = uint32_t(poolSizeCount);
    descrPoolInfo.pPoolSizes = poolSizes;

    // scene pool
    result = vkCreateDescriptorPool(m_device, &descrPoolInfo, m_allocator, &descrPool);
    assert(result == VK_SUCCESS);
    pool = descrPool;

    for (uint32_t n = 0; n < maxSets; n++) {
      VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
      allocInfo.descriptorPool = descrPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &layout;

      result = vkAllocateDescriptorSets(m_device, &allocInfo, sets + n);
      assert(result == VK_SUCCESS);
    }
  }

  VkResult DeviceUtils::allocMemAndBindBuffer(VkBuffer obj, const VkPhysicalDeviceMemoryProperties& memoryProperties, VkDeviceMemory &gpuMem, VkFlags memProps)
  {
    VkResult result;

    VkMemoryRequirements  memReqs;
    vkGetBufferMemoryRequirements(m_device, obj, &memReqs);

    VkMemoryAllocateInfo  memInfo;
    if (!PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(memoryProperties, memReqs, memProps, memInfo)) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }

    result = vkAllocateMemory(m_device, &memInfo, nullptr, &gpuMem);
    if (result != VK_SUCCESS) {
      return result;
    }

    result = vkBindBufferMemory(m_device, obj, gpuMem, 0);
    if (result != VK_SUCCESS) {
      return result;
    }

    return VK_SUCCESS;
  }

  VkBuffer DeviceUtils::createBuffer(size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags) const
  {
    VkResult result;
    VkBuffer buffer;
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.flags = flags;

    result = vkCreateBuffer(m_device, &bufferInfo, m_allocator, &buffer);
    assert(result == VK_SUCCESS);

    return buffer;
  }

  VkBufferView DeviceUtils::createBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize size, VkDeviceSize offset, VkBufferViewCreateFlags flags) const
  {
    VkResult result;

    VkBufferViewCreateInfo  info = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    info.buffer = buffer;
    info.flags = flags;
    info.offset = offset;
    info.range = size;
    info.format = format;

    assert(size);

    VkBufferView view;
    result = vkCreateBufferView(m_device, &info, m_allocator, &view);
    assert(result == VK_SUCCESS);
    return view;
  }

  VkBufferView DeviceUtils::createBufferView(VkDescriptorBufferInfo dinfo, VkFormat format, VkBufferViewCreateFlags flags) const
  {
    VkResult result;

    VkBufferViewCreateInfo  info = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    info.buffer = dinfo.buffer;
    info.flags = flags;
    info.offset = dinfo.offset;
    info.range = dinfo.range;
    info.format = format;

    VkBufferView view;
    result = vkCreateBufferView(m_device, &info, m_allocator, &view);
    assert(result == VK_SUCCESS);
    return view;
  }

  VkDescriptorSetLayout DeviceUtils::createDescriptorSetLayout(size_t numBindings, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSetLayoutCreateFlags flags) const
  {
    VkResult result;
    VkDescriptorSetLayoutCreateInfo descriptorSetEntry = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetEntry.bindingCount = uint32_t(numBindings);
    descriptorSetEntry.pBindings = bindings;
    descriptorSetEntry.flags = flags;

    VkDescriptorSetLayout descriptorSetLayout;
    result = vkCreateDescriptorSetLayout(
      m_device,
      &descriptorSetEntry,
      m_allocator,
      &descriptorSetLayout
    );
    assert(result == VK_SUCCESS);

    return descriptorSetLayout;
  }

  VkPipelineLayout DeviceUtils::createPipelineLayout(size_t numLayouts, const VkDescriptorSetLayout* setLayouts, size_t numRanges, const VkPushConstantRange* ranges) const
  {
    VkResult result;
    VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutCreateInfo.setLayoutCount = uint32_t(numLayouts);
    layoutCreateInfo.pSetLayouts = setLayouts;
    layoutCreateInfo.pushConstantRangeCount = uint32_t(numRanges);
    layoutCreateInfo.pPushConstantRanges = ranges;

    VkPipelineLayout pipelineLayout;
    result = vkCreatePipelineLayout(
      m_device,
      &layoutCreateInfo,
      m_allocator,
      &pipelineLayout
    );
    assert(result == VK_SUCCESS);

    return pipelineLayout;
  }

  //////////////////////////////////////////////////////////////////////////

  void RingFences::init(VkDevice device, const VkAllocationCallbacks* allocator /*= nullptr*/)
  {
    m_allocator = allocator;
    m_device = device;
    m_frame = 0;
    m_waited = 0;
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++) {
      VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
      info.flags = 0;
      VkResult result = vkCreateFence(device, &info, allocator, &m_fences[i]);
    }
  }

  void RingFences::deinit()
  {
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++) {
      vkDestroyFence(m_device, m_fences[i], m_allocator);
    }
  }

  void RingFences::reset()
  {
    vkResetFences(m_device, MAX_RING_FRAMES, m_fences);
    m_frame = 0;
    m_waited = m_frame;
  }

  void RingFences::wait(uint64_t timeout /*= ~0ULL*/)
  {
    if (m_waited == m_frame || m_frame < MAX_RING_FRAMES) {
      return;
    }

    uint32_t waitIndex = (m_frame) % MAX_RING_FRAMES;
    VkResult result = vkWaitForFences(m_device, 1, &m_fences[waitIndex], VK_TRUE, timeout);
    m_waited = m_frame;
  }

  VkFence RingFences::advanceCycle()
  {
    VkFence fence = m_fences[m_frame % MAX_RING_FRAMES];
    vkResetFences(m_device, 1, &fence);
    m_frame++;
    return fence;
  }

  //////////////////////////////////////////////////////////////////////////

  void RingCmdPool::init(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, const VkAllocationCallbacks* allocator)
  {
    m_device = device;
    m_allocator = allocator;
    m_dirty = 0;
    m_index = 0;

    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++) {
      VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
      info.queueFamilyIndex = queueFamilyIndex;
      info.flags = flags;

      VkResult result = vkCreateCommandPool(m_device, &info, m_allocator, &m_cycles[i].pool);
    }
  }

  void RingCmdPool::deinit()
  {
    reset(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++) {
      vkDestroyCommandPool(m_device, m_cycles[i].pool, m_allocator);
    }
  }

  void RingCmdPool::reset(VkCommandPoolResetFlags flags)
  {
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++) {
      Cycle& cycle = m_cycles[i];
      if (m_dirty & (1 << i)) {
        vkFreeCommandBuffers(m_device, cycle.pool, uint32_t(cycle.cmds.size()), cycle.cmds.data());
        vkResetCommandPool(m_device, cycle.pool, flags);
        cycle.cmds.clear();
      }
    }

    m_dirty = 0;
  }

  void RingCmdPool::setCycle(uint32_t cycleIndex)
  {
    if (m_dirty & (1 << cycleIndex)) {
      Cycle& cycle = m_cycles[cycleIndex];
      vkFreeCommandBuffers(m_device, cycle.pool, uint32_t(cycle.cmds.size()), cycle.cmds.data());
      vkResetCommandPool(m_device, cycle.pool, 0);
      cycle.cmds.clear();
      m_dirty &= ~(1 << cycleIndex);
    }
    m_index = cycleIndex;
  }

  VkCommandBuffer RingCmdPool::createCommandBuffer(VkCommandBufferLevel level)
  {
    Cycle& cycle = m_cycles[m_index];

    VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    info.commandBufferCount = 1;
    info.commandPool = cycle.pool;
    info.level = level;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &info, &cmd);

    cycle.cmds.push_back(cmd);
    
    m_dirty |= (1 << m_index);
    return cmd;
  }

  const VkCommandBuffer* RingCmdPool::createCommandBuffers(VkCommandBufferLevel level, uint32_t count)
  {
    Cycle& cycle = m_cycles[m_index];

    VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    info.commandBufferCount = count;
    info.commandPool = cycle.pool;
    info.level = level;

    size_t begin = cycle.cmds.size();
    cycle.cmds.resize(begin + count);
    VkCommandBuffer* cmds = cycle.cmds.data() + begin;
    vkAllocateCommandBuffers(m_device, &info, cmds);

    return cmds;
  }

  //////////////////////////////////////////////////////////////////////////

  void BasicStagingBuffer::init(VkDevice device, const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize chunkSize, const VkAllocationCallbacks* allocator /*= nullptr*/)
  {
    m_device = device;
    m_allocator = allocator;
    m_memoryProperties = memoryProperties;
    m_chunkSize = chunkSize;
    m_available = 0;
    m_used = 0;
    m_buffer = nullptr;
    m_mapping = nullptr;
    m_mem = nullptr;

    allocateBuffer(m_chunkSize);
  }

  void BasicStagingBuffer::deinit()
  {
    if (m_available) {
      vkUnmapMemory(m_device, m_mem);
      vkDestroyBuffer(m_device, m_buffer, m_allocator);
      vkFreeMemory(m_device, m_mem, m_allocator);
      m_buffer = nullptr;
      m_mapping = nullptr;
      m_mem = nullptr;
      m_available = 0;
    }
    m_targetBuffers.clear();
    m_targetBufferCopies.clear();
    m_used = 0;
  }

  void BasicStagingBuffer::allocateBuffer(VkDeviceSize size)
  {
    VkResult result;
    // Create staging buffer
    VkBufferCreateInfo bufferStageInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferStageInfo.size = size;
    bufferStageInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferStageInfo.flags = 0;

    result = vkCreateBuffer(m_device, &bufferStageInfo, m_allocator, &m_buffer);

    VkMemoryRequirements  memReqs;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memReqs);

    VkMemoryAllocateInfo  memInfo;
    PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(*m_memoryProperties, memReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memInfo);

    result = vkAllocateMemory(m_device, &memInfo, nullptr, &m_mem);
    result = vkBindBufferMemory(m_device, m_buffer, m_mem, 0);
    result = vkMapMemory(m_device, m_mem, 0, size, 0, (void**)&m_mapping);

    m_available = size;
    m_used = 0;
  }

  void BasicStagingBuffer::enqueue(VkImage image, const VkOffset3D &offset, const VkExtent3D &extent, const VkImageSubresourceLayers &subresource, VkDeviceSize size, const void* data)
  {
    if (m_used + size > m_available) {
      assert(m_used == 0 && "forgot to flush prior enqueue");

      if (m_available) {
        deinit();
      }

      allocateBuffer(std::max(size, m_chunkSize));
    }

    memcpy(m_mapping + m_used, data, size);

    VkBufferImageCopy cpy;
    cpy.bufferOffset = m_used;
    cpy.bufferRowLength = 0;
    cpy.bufferImageHeight = 0;
    cpy.imageSubresource = subresource;
    cpy.imageOffset = offset;
    cpy.imageExtent = extent;

    m_targetImages.push_back(image);
    m_targetImageCopies.push_back(cpy);

    m_used += size;
  }

  void BasicStagingBuffer::enqueue(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
  {
    if (m_used + size > m_available) {
      assert(m_used == 0 && "forgot to flush prior enqueue");

      if (m_available) {
        deinit();
      }

      allocateBuffer(std::max(size, m_chunkSize));
    }

    memcpy(m_mapping + m_used, data, size);

    VkBufferCopy cpy;
    cpy.size = size;
    cpy.srcOffset = m_used;
    cpy.dstOffset = offset;

    m_targetBuffers.push_back(buffer);
    m_targetBufferCopies.push_back(cpy);

    m_used += size;
  }

  void BasicStagingBuffer::flush(VkCommandBuffer cmd)
  {
    for (size_t i = 0; i < m_targetImages.size(); i++) {
      vkCmdCopyBufferToImage(cmd, m_buffer, m_targetImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &m_targetImageCopies[i]);
    }
    // TODO could sort by target buffers and use regions etc...
    for (size_t i = 0; i < m_targetBuffers.size(); i++) {
      vkCmdCopyBuffer(cmd, m_buffer, m_targetBuffers[i], 1, &m_targetBufferCopies[i]);
    }

    m_targetImages.clear();
    m_targetImageCopies.clear();
    m_targetBuffers.clear();
    m_targetBufferCopies.clear();
    m_used = 0;
  }

  void BasicStagingBuffer::flush(TempSubmissionInterface* tempIF, bool sync)
  {
    if (!canFlush()) return;

    VkCommandBuffer cmd = tempIF->tempSubmissionCreateCommandBuffer(true);
    VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin);
    flush(cmd);
    vkEndCommandBuffer(cmd);

    tempIF->tempSubmissionEnqueue(cmd);
    tempIF->tempSubmissionSubmit(sync);
  }

  //////////////////////////////////////////////////////////////////////////

  nv_helpers_vk::AllocationID BasicDeviceMemoryAllocator::createID(Allocation& allocation)
  {
    // find free slot
    for (size_t i = 0; i < m_allocations.size(); i++) {
      if (!m_allocations[i].id.isValid()) {
        m_allocations[i].id.index = (uint32_t)i;
        m_allocations[i].id.incarnation++;
        m_allocations[i].allocation = allocation;
        return m_allocations[i].id;
      }
    }

    // otherwise push to end
    AllocationInfo info;
    info.id.index = (uint32_t)m_allocations.size();
    info.id.incarnation = 0;
    info.allocation = allocation;

    m_allocations.push_back(info);

    return info.id;
  }  

  void BasicDeviceMemoryAllocator::freeID(AllocationID id)
  {
    assert(m_allocations[id.index].id.isEqual(id));
    // set to invalid
    m_allocations[id.index].id.invalidate();

    // shrink if last
    if ((size_t)id.index == m_allocations.size() - 1) {
      m_allocations.pop_back();
    }
  }

  nv_helpers_vk::BasicDeviceMemoryAllocator::MemoryBlockIterator BasicDeviceMemoryAllocator::getBlock(const Allocation& allocation)
  {
    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
    {
      MemoryBlock &block = *it;

      if (block.mem == allocation.mem)
      {
        return it;
      }
    }

    return m_blocks.end();
  }

  void BasicDeviceMemoryAllocator::init(VkDevice device, const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize, const VkAllocationCallbacks* allocator)
  {
    m_device = device;
    m_allocation_callbacks = allocator;
    m_memoryProperties = memoryProperties;
    m_blockSize = blockSize;
  }

  void BasicDeviceMemoryAllocator::deinit()
  {

    for (const auto& it : m_blocks)
    {
      if (it.mapped) {
        vkUnmapMemory(m_device, it.mem);
      }
      vkFreeMemory(m_device, it.mem, m_allocation_callbacks);
    }

    m_allocations.clear();
    m_blocks.clear();
  }

  float BasicDeviceMemoryAllocator::getUtilization(VkDeviceSize& allocatedSize, VkDeviceSize& usedSize) const
  {
    allocatedSize = m_allocatedSize;
    usedSize = m_usedSize;

    return float(double(usedSize) / double(allocatedSize));
  }

  nv_helpers_vk::Allocation BasicDeviceMemoryAllocator::getAllocation(AllocationID id) const
  {
    assert(m_allocations[id.index].id.isEqual(id));
    return m_allocations[id.index].allocation;
  }

  const VkPhysicalDeviceMemoryProperties* BasicDeviceMemoryAllocator::getMemoryProperties() const
  {
    return m_memoryProperties;
  }

  AllocationID BasicDeviceMemoryAllocator::alloc(const VkMemoryRequirements &memReqs, VkMemoryPropertyFlags memProps, const VkMemoryDedicatedAllocateInfo* dedicated)
  {
    VkMemoryAllocateInfo memInfo;

    // Fill out allocation info structure
    if (!PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(*m_memoryProperties, memReqs, memProps, memInfo))
    {
      return AllocationID();
    }

    if (!dedicated) {
      // First try to find an existing memory block that we can use
      for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
      {
        MemoryBlock &block = *it;

        // Ignore blocks with the wrong memory type
        if (block.memoryTypeIndex != memInfo.memoryTypeIndex)
        {
          continue;
        }

        VkDeviceSize offset = block.currentOffset;

        // Align offset to requirement
        offset = ((offset + memReqs.alignment - 1) / memReqs.alignment) * memReqs.alignment;

        // Look for a block which has enough free space available
        if (offset + memReqs.size < block.allocationSize)
        {
          block.currentOffset = offset + memInfo.allocationSize;
          block.allocationCount++;

          Allocation allocation;
          allocation.mem = block.mem;
          allocation.offset = offset;
          allocation.size = memReqs.size;

          m_usedSize += allocation.size;

          return createID(allocation);
        }
      }
    }

    // Could not find enough space in an existing block, so allocate a new one
    m_blocks.emplace_back();
    MemoryBlock& block = m_blocks.back();

    block.allocationSize = memInfo.allocationSize = std::max(m_blockSize, memReqs.size);
    block.memoryTypeIndex = memInfo.memoryTypeIndex;

    if (dedicated) {
      block.allocationSize = memReqs.size;
      memInfo.pNext = dedicated;
    }

    const VkResult res = vkAllocateMemory(m_device, &memInfo, m_allocation_callbacks, &block.mem);

    if (res == VK_SUCCESS)
    {
      m_allocatedSize += block.allocationSize;

      block.currentOffset = memReqs.size;
      block.allocationCount = 1;
      block.mapCount = 0;
      block.mapped = nullptr;
      block.mappable = (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

      Allocation allocation;
      allocation.mem = block.mem;
      allocation.size = memReqs.size;

      m_usedSize += allocation.size;

      return createID(allocation);
    }
    else
    {
      m_blocks.pop_back();

      return AllocationID();
    }
  }

  void BasicDeviceMemoryAllocator::free(AllocationID allocationID)
  {
    Allocation allocation = getAllocation(allocationID);
    freeID(allocationID);
    
    m_usedSize -= allocation.size;
    
    auto it = getBlock(allocation);
    assert(it != m_blocks.end());

    MemoryBlock &block = *it;
    if (--block.allocationCount == 0)
    {
      assert(!block.mapped);
      vkFreeMemory(m_device, block.mem, m_allocation_callbacks);

      m_allocatedSize -= block.allocationSize;

      m_blocks.erase(it);
    }
  }

  void* BasicDeviceMemoryAllocator::map(AllocationID allocationID)
  {
    Allocation allocation = getAllocation(allocationID);
    auto it = getBlock(allocation);
    assert(it != m_blocks.end());

    MemoryBlock &block = *it;
    assert(block.mappable);
    block.mapCount++;
    if (!block.mapped) {
      VkResult result = vkMapMemory(m_device, block.mem, 0, block.allocationSize, 0, (void**)&block.mapped);
      assert(result == VK_SUCCESS);
    }
    return block.mapped + allocation.offset;
  }

  void BasicDeviceMemoryAllocator::unmap(AllocationID allocationID)
  {
    Allocation allocation = getAllocation(allocationID);
    auto it = getBlock(allocation);
    assert(it != m_blocks.end());

    MemoryBlock &block = *it;
    assert(block.mapped);
    if (--block.mapCount == 0) {
      block.mapped = nullptr;
      vkUnmapMemory(m_device, block.mem);
    }
  }

  VkResult BasicDeviceMemoryAllocator::create(const VkImageCreateInfo &inCreateInfo, VkImage &image, AllocationID &allocationID, VkMemoryPropertyFlags memProps, bool useDedicated)
  {
    VkImageCreateInfo createInfo = inCreateInfo;

    VkDedicatedAllocationImageCreateInfoNV  dedicatedImage = { VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV };
    dedicatedImage.pNext = createInfo.pNext;
    dedicatedImage.dedicatedAllocation = VK_TRUE;
    if (useDedicated) {
      createInfo.pNext = &dedicatedImage;
    }

    VkResult result = vkCreateImage(m_device, &createInfo, m_allocation_callbacks, &image);
    if (result != VK_SUCCESS)
      return result;


    VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkMemoryDedicatedRequirements dedicatedRegs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
    VkImageMemoryRequirementsInfo2  imageReqs = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2 };
    if (useDedicated) {
      imageReqs.image = image;
      memReqs.pNext = &dedicatedRegs;
      vkGetImageMemoryRequirements2(m_device, &imageReqs, &memReqs);
    }
    else {
      vkGetImageMemoryRequirements(m_device, image, &memReqs.memoryRequirements);
    }    

    VkMemoryDedicatedAllocateInfo dedicatedInfo = { VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV };
    dedicatedInfo.image = image;

    allocationID = alloc(memReqs.memoryRequirements, memProps, useDedicated ? &dedicatedInfo : nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if (allocation.mem == VK_NULL_HANDLE) {
      vkDestroyImage(m_device, image, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    result = vkBindImageMemory(m_device, image, allocation.mem, allocation.offset);
    if (result != VK_SUCCESS) {
      vkDestroyImage(m_device, image, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }
  VkResult BasicDeviceMemoryAllocator::create(const VkBufferCreateInfo &inCreateInfo, VkBuffer &buffer, AllocationID &allocationID, VkMemoryPropertyFlags memProps, bool useDedicated)
  {
    VkBufferCreateInfo createInfo = inCreateInfo;

    VkDedicatedAllocationBufferCreateInfoNV  dedicatedBuffer = { VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV };
    dedicatedBuffer.pNext = createInfo.pNext;
    dedicatedBuffer.dedicatedAllocation = VK_TRUE;
    if (useDedicated) {
      createInfo.pNext = &dedicatedBuffer;
    }

    VkResult result = vkCreateBuffer(m_device, &createInfo, m_allocation_callbacks, &buffer);
    if (result != VK_SUCCESS) {
      return result;
    }

    VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkMemoryDedicatedRequirements dedicatedRegs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
    VkBufferMemoryRequirementsInfo2  bufferReqs = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };
    if (useDedicated) {
      bufferReqs.buffer = buffer;
      memReqs.pNext = &dedicatedRegs;
      vkGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);
    }
    else {
      vkGetBufferMemoryRequirements(m_device, buffer, &memReqs.memoryRequirements);
    }
    
    VkMemoryDedicatedAllocateInfo dedicatedInfo = { VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV };
    dedicatedInfo.buffer = buffer;

    allocationID = alloc(memReqs.memoryRequirements, memProps, useDedicated ? &dedicatedInfo : nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if (allocation.mem == VK_NULL_HANDLE) {
      vkDestroyBuffer(m_device, buffer, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    result = vkBindBufferMemory(m_device, buffer, allocation.mem, allocation.offset);
    if (result != VK_SUCCESS) {
      vkDestroyBuffer(m_device, buffer, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }

#if VK_NVX_raytracing
  VkResult BasicDeviceMemoryAllocator::create(const VkAccelerationStructureCreateInfoNVX &createInfo, VkAccelerationStructureNVX &accel, AllocationID &allocationID, VkMemoryPropertyFlags memProps /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/, bool useDedicated /*= false*/)
  {
    VkResult result = vkCreateAccelerationStructureNVX(m_device, &createInfo, m_allocation_callbacks, &accel);
    if (result != VK_SUCCESS) {
      return result;
    }

    VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkAccelerationStructureMemoryRequirementsInfoNVX memInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NVX };
    memInfo.accelerationStructure = accel;
    vkGetAccelerationStructureMemoryRequirementsNVX(m_device, &memInfo, &memReqs);

    allocationID = alloc(memReqs.memoryRequirements, memProps, nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if (allocation.mem == VK_NULL_HANDLE) {
      vkDestroyAccelerationStructureNVX(m_device, accel, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    VkBindAccelerationStructureMemoryInfoNVX bind = { VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NVX };
    bind.accelerationStructure = accel;
    bind.memory = allocation.mem;
    bind.memoryOffset = allocation.offset;

    result = vkBindAccelerationStructureMemoryNVX(m_device, 1, &bind);
    if (result != VK_SUCCESS) {
      vkDestroyAccelerationStructureNVX(m_device, accel, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }
#endif

#if VK_NV_ray_tracing
  VkResult BasicDeviceMemoryAllocator::create(const VkAccelerationStructureCreateInfoNV &createInfo, VkAccelerationStructureNV &accel, AllocationID &allocationID, VkMemoryPropertyFlags memProps /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/, bool useDedicated /*= false*/)
  {
    VkResult result = vkCreateAccelerationStructureNV(m_device, &createInfo, m_allocation_callbacks, &accel);
    if (result != VK_SUCCESS) {
      return result;
    }

    VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    VkAccelerationStructureMemoryRequirementsInfoNV memInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV };
    memInfo.accelerationStructure = accel;
    vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memInfo, &memReqs);

    allocationID = alloc(memReqs.memoryRequirements, memProps, nullptr);
    Allocation allocation = allocationID.isValid() ? getAllocation(allocationID) : Allocation();

    if (allocation.mem == VK_NULL_HANDLE) {
      vkDestroyAccelerationStructureNV(m_device, accel, m_allocation_callbacks);
      return VK_ERROR_OUT_OF_POOL_MEMORY;
    }

    VkBindAccelerationStructureMemoryInfoNV bind = { VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV };
    bind.accelerationStructure = accel;
    bind.memory = allocation.mem;
    bind.memoryOffset = allocation.offset;

    result = vkBindAccelerationStructureMemoryNV(m_device, 1, &bind);
    if (result != VK_SUCCESS) {
      vkDestroyAccelerationStructureNV(m_device, accel, m_allocation_callbacks);
      return result;
    }

    return VK_SUCCESS;
  }
#endif

  //////////////////////////////////////////////////////////////////////////

  void MemoryBlockManager::init(const VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize blockSize /*= 32 * 1024 * 1024*/)
  {
    m_memoryProperties = memoryProperties;
    m_blockSize = blockSize;
    m_blocks.clear();
  }

  MemoryBlockManager::Entry MemoryBlockManager::add(const VkMemoryRequirements &memReqs, VkMemoryPropertyFlags memProps /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
  {
    MemoryBlockManager::Entry entry;

    VkMemoryAllocateInfo memInfo;

    // Fill out allocation info structure
    if (!PhysicalDeviceMemoryProperties_getMemoryAllocationInfo(*m_memoryProperties, memReqs, memProps, memInfo)){
      return entry;
    }

    // find open block

    size_t idx = 0;
    for (auto& block : m_blocks) {

      if (block.memAllocate.memoryTypeIndex != memInfo.memoryTypeIndex || block.memAllocate.allocationSize > m_blockSize) {
        idx++;
        continue;
      }

      VkDeviceSize offset = block.memAllocate.allocationSize;

      // Align offset to requirement
      offset = ((offset + memReqs.alignment - 1) / memReqs.alignment) * memReqs.alignment;

      // update the three allocation infos, whatever the user ends up using
      block.memAllocate.allocationSize = offset + memReqs.size;
      block.memReqs.size = block.memAllocate.allocationSize;
      block.memProps |= memProps;
    
      entry.blockIdx = idx;
      entry.offset = offset;

      return entry;
    }

    Block block;
    block.memAllocate = memInfo;
    block.memProps = memProps;
    block.memReqs = memReqs;

    entry.blockIdx = m_blocks.size();
    entry.offset = 0;

    m_blocks.push_back(block);

    return entry;
  }

  void MemoryBlockManager::clear()
  {
    m_blocks.clear();
  }

  const size_t MemoryBlockManager::getBlockCount() const
  {
    return m_blocks.size();
  }

  const nv_helpers_vk::MemoryBlockManager::Block& MemoryBlockManager::getBlock(size_t idx) const
  {
    return m_blocks[idx];
  }

  VkBindImageMemoryInfo MemoryBlockManager::makeBind(VkImage image, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const
  {
    const Allocation& allocation = blockAllocations[entry.blockIdx];
    VkBindImageMemoryInfo info = { VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO };
    info.image = image;
    info.memory = allocation.mem;
    info.memoryOffset = allocation.offset + entry.offset;
    return info;
  }

  VkBindBufferMemoryInfo MemoryBlockManager::makeBind(VkBuffer buffer, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const
  {
    const Allocation& allocation = blockAllocations[entry.blockIdx];
    VkBindBufferMemoryInfo info = { VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO };
    info.buffer = buffer;
    info.memory = allocation.mem;
    info.memoryOffset = allocation.offset + entry.offset;
    return info;
  }

#if VK_NVX_raytracing
  VkBindAccelerationStructureMemoryInfoNVX MemoryBlockManager::makeBind(VkAccelerationStructureNVX accel, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const
  {
    const Allocation& allocation = blockAllocations[entry.blockIdx];
    VkBindAccelerationStructureMemoryInfoNVX info = { VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NVX };
    info.accelerationStructure = accel;
    info.memory = allocation.mem;
    info.memoryOffset = allocation.offset + entry.offset;
    return info;
  }
#endif

#if VK_NV_ray_tracing
  VkBindAccelerationStructureMemoryInfoNV MemoryBlockManager::makeBind(VkAccelerationStructureNV accel, const MemoryBlockManager::Entry& entry, const Allocation* blockAllocations) const
  {
    const Allocation& allocation = blockAllocations[entry.blockIdx];
    VkBindAccelerationStructureMemoryInfoNV info = { VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV };
    info.accelerationStructure = accel;
    info.memory = allocation.mem;
    info.memoryOffset = allocation.offset + entry.offset;
    return info;
  }
#endif
}

