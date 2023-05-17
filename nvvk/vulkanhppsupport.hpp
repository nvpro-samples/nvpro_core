/*
 * SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "commands_vk.hpp"
#include "descriptorsets_vk.hpp"
#include "error_vk.hpp"
#include "images_vk.hpp"
#include "pipeline_vk.hpp"
#include "raytraceKHR_vk.hpp"
#include "raytraceNV_vk.hpp"
#include "renderpasses_vk.hpp"
#include "resourceallocator_vk.hpp"
#include "samplers_vk.hpp"

#include <vulkan/vulkan.hpp>

namespace nvvk {
bool checkResult(vk::Result result, const char* message);
bool checkResult(vk::Result result, const char* file, int32_t line);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nvvkpp {
class CommandPool : public nvvk::CommandPool
{
public:
  CommandPool(CommandPool const&) = delete;
  CommandPool& operator=(CommandPool const&) = delete;

  CommandPool() {}
  ~CommandPool() { deinit(); }

  // if defaultQueue is null, uses first queue from familyIndex as default
  CommandPool(VkDevice                 device,
              uint32_t                 familyIndex,
              VkCommandPoolCreateFlags flags        = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
              VkQueue                  defaultQueue = VK_NULL_HANDLE)
  {
    nvvk::CommandPool::init(device, familyIndex, flags, defaultQueue);
  }

  CommandPool(VkDevice device, uint32_t familyIndex, vk::CommandPoolCreateFlags flags, vk::Queue defaultQueue = nullptr)
  {
    nvvk::CommandPool::init(device, familyIndex, (VkCommandPoolCreateFlags)flags, defaultQueue);
  }

  void init(vk::Device device, uint32_t familyIndex, vk::CommandPoolCreateFlags flags, vk::Queue defaultQueue = nullptr)
  {
    nvvk::CommandPool::init(device, familyIndex, (VkCommandPoolCreateFlags)flags, defaultQueue);
  }


  // ensure proper cycle is set prior this
  VkCommandBuffer createCommandBuffer(vk::CommandBufferLevel      level = vk::CommandBufferLevel::ePrimary,
                                      bool                        begin = true,
                                      vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
                                      const vk::CommandBufferInheritanceInfo* pInheritanceInfo = nullptr)
  {
    return nvvk::CommandPool::createCommandBuffer((VkCommandBufferLevel)level, begin, (VkCommandBufferUsageFlags)flags,
                                                  (const VkCommandBufferInheritanceInfo*)pInheritanceInfo);
  }

  void submit(vk::ArrayProxy<vk::CommandBuffer>& cmds, vk::Fence fence = vk::Fence())
  {
    nvvk::CommandPool::submit(cmds.size(), &static_cast<const VkCommandBuffer&>(*cmds.data()), m_queue, fence);
  }

  void destroy(size_t count, const vk::CommandBuffer* cmds)
  {
    nvvk::CommandPool::destroy(count, (const VkCommandBuffer*)cmds);
  }
  void destroy(const std::vector<vk::CommandBuffer>& cmds)
  {
    nvvk::CommandPool::destroy(cmds.size(), (const VkCommandBuffer*)cmds.data());
  }
  void submitAndWait(size_t count, const vk::CommandBuffer* cmds, VkQueue queue)
  {
    nvvk::CommandPool::submitAndWait(count, (const VkCommandBuffer*)cmds, queue);
  }
  void submitAndWait(const std::vector<vk::CommandBuffer>& cmds, VkQueue queue)
  {
    nvvk::CommandPool::submitAndWait(cmds.size(), (const VkCommandBuffer*)cmds.data(), queue);
  }
  void submitAndWait(size_t count, const vk::CommandBuffer* cmds)
  {
    nvvk::CommandPool::submitAndWait(count, (const VkCommandBuffer*)cmds, m_queue);
  }
  void submitAndWait(const std::vector<vk::CommandBuffer>& cmds)
  {
    nvvk::CommandPool::submitAndWait(cmds.size(), (const VkCommandBuffer*)cmds.data(), m_queue);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ScopeCommandBuffer : public CommandPool
{
public:
  // if queue is null, uses first queue from familyIndex
  ScopeCommandBuffer(vk::Device device, uint32_t familyIndex, vk::Queue queue = VK_NULL_HANDLE)
  {
    CommandPool::init(device, familyIndex, vk::CommandPoolCreateFlagBits::eTransient, queue);
    m_cmd = createCommandBuffer(vk::CommandBufferLevel::ePrimary);
  }

  ~ScopeCommandBuffer() { nvvk::CommandPool::submitAndWait(m_cmd); }

  operator VkCommandBuffer() const { return m_cmd; };
  operator vk::CommandBuffer() const { return (vk::CommandBuffer)m_cmd; };

private:
  VkCommandBuffer m_cmd;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class RingCommandPool : public nvvk::RingCommandPool
{
public:
  RingCommandPool(RingCommandPool const&) = delete;
  RingCommandPool& operator=(RingCommandPool const&) = delete;

  RingCommandPool(VkDevice                 device,
                  uint32_t                 queueFamilyIndex,
                  VkCommandPoolCreateFlags flags    = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                  uint32_t                 ringSize = nvvk::DEFAULT_RING_SIZE)
  {
    nvvk::RingCommandPool::init(device, queueFamilyIndex, flags, ringSize);
  }
  RingCommandPool() {}
  ~RingCommandPool() { deinit(); }

  void init(vk::Device device, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags, uint32_t ringSize = nvvk::DEFAULT_RING_SIZE)
  {
    nvvk::RingCommandPool::init(device, queueFamilyIndex, (VkCommandPoolCreateFlags)flags, ringSize);
  }
  RingCommandPool(vk::Device device, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags, uint32_t ringSize = nvvk::DEFAULT_RING_SIZE)
  {
    nvvk::RingCommandPool::init(device, queueFamilyIndex, (VkCommandPoolCreateFlags)flags, ringSize);
  }

  // ensure proper cycle is set prior this
  VkCommandBuffer createCommandBuffer(vk::CommandBufferLevel      level,
                                      bool                        begin = true,
                                      vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
                                      const vk::CommandBufferInheritanceInfo* pInheritanceInfo = nullptr)
  {
    return nvvk::RingCommandPool::createCommandBuffer((VkCommandBufferLevel)level, begin, (VkCommandBufferUsageFlags)flags,
                                                      (const VkCommandBufferInheritanceInfo*)pInheritanceInfo);
  }

  // pointer is only valid until next create
  const vk::CommandBuffer* createCommandBuffers(vk::CommandBufferLevel level, uint32_t count)
  {
    return (const vk::CommandBuffer*)nvvk::RingCommandPool::createCommandBuffers((VkCommandBufferLevel)level, count);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BatchSubmission : public nvvk::BatchSubmission
{
public:
  BatchSubmission(BatchSubmission const&) = delete;
  BatchSubmission& operator=(BatchSubmission const&) = delete;

  BatchSubmission() {}
  BatchSubmission(VkQueue queue) { init(queue); }

  void enqueue(uint32_t num, const vk::CommandBuffer* cmdbuffers)
  {
    nvvk::BatchSubmission::enqueue(num, (const VkCommandBuffer*)cmdbuffers);
  }
  void enqueueWait(vk::Semaphore sem, vk::PipelineStageFlags flag)
  {
    nvvk::BatchSubmission::enqueueWait(sem, (VkPipelineStageFlags)flag);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FencedCommandPools : public nvvk::FencedCommandPools
{
public:
  FencedCommandPools(FencedCommandPools const&) = delete;
  FencedCommandPools& operator=(FencedCommandPools const&) = delete;

  FencedCommandPools() {}
  ~FencedCommandPools() { nvvk::FencedCommandPools::deinit(); }

  FencedCommandPools(VkDevice                 device,
                     VkQueue                  queue,
                     uint32_t                 queueFamilyIndex,
                     VkCommandPoolCreateFlags flags    = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                     uint32_t                 ringSize = nvvk::DEFAULT_RING_SIZE)
  {
    nvvk::FencedCommandPools::init(device, queue, queueFamilyIndex, flags, ringSize);
  }

  FencedCommandPools(vk::Device device, vk::Queue queue, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags, uint32_t ringSize = nvvk::DEFAULT_RING_SIZE)
  {
    nvvk::FencedCommandPools::init(device, queue, queueFamilyIndex, (VkCommandPoolCreateFlags)flags, ringSize);
  }
  void init(vk::Device device, vk::Queue queue, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags, uint32_t ringSize = nvvk::DEFAULT_RING_SIZE)
  {
    nvvk::FencedCommandPools::init(device, queue, queueFamilyIndex, (VkCommandPoolCreateFlags)flags, ringSize);
  }

  void enqueue(vk::CommandBuffer cmdbuffer)
  {
    BatchSubmission::enqueue(cmdbuffer);
  }
  void enqueue(uint32_t num, const vk::CommandBuffer* cmdbuffers)
  {
    nvvk::FencedCommandPools::enqueue(num, (const VkCommandBuffer*)cmdbuffers);
  }
  void enqueueWait(vk::Semaphore sem, vk::PipelineStageFlags flag)
  {
    nvvk::FencedCommandPools::enqueueWait(sem, (VkPipelineStageFlags)flag);
  }


  // ensure proper cycle is set prior this
  VkCommandBuffer createCommandBuffer(vk::CommandBufferLevel      level = vk::CommandBufferLevel::ePrimary,
                                      bool                        begin = true,
                                      vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
                                      const vk::CommandBufferInheritanceInfo* pInheritanceInfo = nullptr)
  {
    return nvvk::FencedCommandPools::createCommandBuffer((VkCommandBufferLevel)level, begin, (VkCommandBufferUsageFlags)flags,
                                                         (const VkCommandBufferInheritanceInfo*)pInheritanceInfo);
  }

  // pointer is only valid until next create
  const vk::CommandBuffer* createCommandBuffers(vk::CommandBufferLevel level, uint32_t count)
  {
    return (const vk::CommandBuffer*)nvvk::FencedCommandPools::createCommandBuffers((VkCommandBufferLevel)level, count);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  struct ScopedCmd
  {
    FencedCommandPools* pCmdPools;
    VkCommandBuffer     cmd;

    ScopedCmd(FencedCommandPools& cp)
    {
      pCmdPools = &cp;
      cmd       = cp.nvvk::FencedCommandPools::createCommandBuffer();
    }
    ~ScopedCmd()
    {
      vkEndCommandBuffer(cmd);
      pCmdPools->nvvk::FencedCommandPools::enqueue(cmd);
      pCmdPools->execute();
      pCmdPools->waitIdle();
    }

    operator VkCommandBuffer() { return cmd; }

    operator vk::CommandBuffer() const { return (vk::CommandBuffer)cmd; };
  };
};

inline VkDescriptorPool createDescriptorPool(vk::Device device, const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets)
{
  return nvvk::createDescriptorPool(device, poolSizes.size(),
                                    reinterpret_cast<const VkDescriptorPoolSize*>(poolSizes.data()), maxSets);
}


inline VkDescriptorSet allocateDescriptorSet(vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout)
{
  return nvvk::allocateDescriptorSet(device, pool, layout);
}

inline void allocateDescriptorSets(vk::Device                      device,
                                   vk::DescriptorPool              pool,
                                   vk::DescriptorSetLayout         layout,
                                   uint32_t                        count,
                                   std::vector<vk::DescriptorSet>& sets)
{
  nvvk::allocateDescriptorSets(device, pool, layout, count, reinterpret_cast<std::vector<VkDescriptorSet>&>(sets));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class DescriptorSetBindings : public nvvk::DescriptorSetBindings
{
public:
  DescriptorSetBindings() = default;
  DescriptorSetBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
      : nvvk::DescriptorSetBindings(bindings)
  {
  }

  DescriptorSetBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
  {
    auto source = &static_cast<const VkDescriptorSetLayoutBinding&>(bindings[0]);
    m_bindings.assign(source, source + bindings.size());
  }
  void addBinding(uint32_t binding,  // Slot to which the descriptor will be bound, corresponding to the layout
                  // binding index in the shader
                  vk::DescriptorType   type,        // Type of the bound descriptor(s)
                  uint32_t             count,       // Number of descriptors
                  vk::ShaderStageFlags stageFlags,  // Shader stages at which the bound resources will be available
                  const vk::Sampler*   pImmutableSampler = nullptr  // Corresponding sampler, in case of textures
  )
  {
    m_bindings.push_back({binding, static_cast<VkDescriptorType>(type), count, static_cast<VkShaderStageFlags>(stageFlags),
                          reinterpret_cast<const VkSampler*>(pImmutableSampler)});
  }
  void addBinding(const vk::DescriptorSetLayoutBinding& layoutBinding)
  {
    m_bindings.emplace_back(static_cast<VkDescriptorSetLayoutBinding>(layoutBinding));
  }
  void setBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
  {
    nvvk::DescriptorSetBindings::setBindings(reinterpret_cast<const std::vector<VkDescriptorSetLayoutBinding>&>(bindings));
  }

  void setBindingFlags(uint32_t binding, vk::DescriptorBindingFlags bindingFlags)
  {
    nvvk::DescriptorSetBindings::setBindingFlags(binding, static_cast<VkDescriptorBindingFlags>(bindingFlags));
  }

  void addRequiredPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t numSets) const
  {
    nvvk::DescriptorSetBindings::addRequiredPoolSizes(reinterpret_cast<std::vector<VkDescriptorPoolSize>&>(poolSizes), numSets);
  }

  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet              dstSet,
                                   uint32_t                       dstBinding,
                                   const vk::DescriptorImageInfo* pImageInfo,
                                   uint32_t                       arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(dstSet, dstBinding,
                                                  reinterpret_cast<const VkDescriptorImageInfo*>(pImageInfo), arrayElement);
  }

  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet            dstSet,
                                   uint32_t                     dstBinding,
                                   const VkDescriptorImageInfo* pImageInfo,
                                   uint32_t                     arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(dstSet, dstBinding, pImageInfo, arrayElement);
  }

  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet               dstSet,
                                   uint32_t                        dstBinding,
                                   const vk::DescriptorBufferInfo* pBufferInfo,
                                   uint32_t                        arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(dstSet, dstBinding,
                                                  reinterpret_cast<const VkDescriptorBufferInfo*>(pBufferInfo), arrayElement);
  }
  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet     dstSet,
                                   uint32_t              dstBinding,
                                   const vk::BufferView* pTexelBufferView,
                                   uint32_t              arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(dstSet, dstBinding,
                                                  reinterpret_cast<const VkBufferView*>(pTexelBufferView), arrayElement);
  }
#if VK_NV_ray_tracing
  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet                                    dstSet,
                                   uint32_t                                             dstBinding,
                                   const vk::WriteDescriptorSetAccelerationStructureNV* pAccel,
                                   uint32_t                                             arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(
        dstSet, dstBinding, reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureNV*>(pAccel), arrayElement);
  }
#endif
#if VK_KHR_acceleration_structure
  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet                                     dstSet,
                                   uint32_t                                              dstBinding,
                                   const vk::WriteDescriptorSetAccelerationStructureKHR* pAccel,
                                   uint32_t                                              arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(
        dstSet, dstBinding, reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(pAccel), arrayElement);
  }
#endif
#if VK_EXT_inline_uniform_block
  vk::WriteDescriptorSet makeWrite(vk::DescriptorSet                                  dstSet,
                                   uint32_t                                           dstBinding,
                                   const vk::WriteDescriptorSetInlineUniformBlockEXT* pInlineUniform,
                                   uint32_t                                           arrayElement = 0) const
  {
    return nvvk::DescriptorSetBindings::makeWrite(
        dstSet, dstBinding, reinterpret_cast<const VkWriteDescriptorSetInlineUniformBlockEXT*>(pInlineUniform), arrayElement);
  }
#endif
  vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding, const vk::DescriptorImageInfo* pImageInfo) const
  {
    return nvvk::DescriptorSetBindings::makeWriteArray(dstSet, dstBinding,
                                                       reinterpret_cast<const VkDescriptorImageInfo*>(pImageInfo));
  }
  vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding, const vk::DescriptorBufferInfo* pBufferInfo) const
  {
    return nvvk::DescriptorSetBindings::makeWriteArray(dstSet, dstBinding,
                                                       reinterpret_cast<const VkDescriptorBufferInfo*>(pBufferInfo));
  }
  vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet dstSet, uint32_t dstBinding, const vk::BufferView* pTexelBufferView) const
  {
    return nvvk::DescriptorSetBindings::makeWriteArray(dstSet, dstBinding, reinterpret_cast<const VkBufferView*>(pTexelBufferView));
  }
#if VK_NV_ray_tracing
  vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet                                    dstSet,
                                        uint32_t                                             dstBinding,
                                        const vk::WriteDescriptorSetAccelerationStructureNV* pAccel) const
  {
    return nvvk::DescriptorSetBindings::makeWriteArray(
        dstSet, dstBinding, reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureNV*>(pAccel));
  }
#endif
#if VK_KHR_acceleration_structure
  vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet                                     dstSet,
                                        uint32_t                                              dstBinding,
                                        const vk::WriteDescriptorSetAccelerationStructureKHR* pAccel) const
  {
    return nvvk::DescriptorSetBindings::makeWriteArray(
        dstSet, dstBinding, reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(pAccel));
  }
#endif
#if VK_EXT_inline_uniform_block
  vk::WriteDescriptorSet makeWriteArray(vk::DescriptorSet                                  dstSet,
                                        uint32_t                                           dstBinding,
                                        const vk::WriteDescriptorSetInlineUniformBlockEXT* pInline) const
  {
    return nvvk::DescriptorSetBindings::makeWriteArray(
        dstSet, dstBinding, reinterpret_cast<const VkWriteDescriptorSetInlineUniformBlockEXT*>(pInline));
  }
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DescriptorSetContainer : public nvvk::DescriptorSetContainer
{
public:
  DescriptorSetContainer(DescriptorSetContainer const&) = delete;
  DescriptorSetContainer& operator=(DescriptorSetContainer const&) = delete;

  DescriptorSetContainer() {}
  DescriptorSetContainer(VkDevice device)
      : nvvk::DescriptorSetContainer(device){};

  void addBinding(uint32_t binding,  // Slot to which the descriptor will be bound, corresponding to the layout
                  // binding index in the shader
                  vk::DescriptorType   type,        // Type of the bound descriptor(s)
                  uint32_t             count,       // Number of descriptors
                  vk::ShaderStageFlags stageFlags,  // Shader stages at which the bound resources will be available
                  const vk::Sampler*   pImmutableSampler = nullptr  // Corresponding sampler, in case of textures
  )
  {
    m_bindings.addBinding({binding, static_cast<VkDescriptorType>(type), count, static_cast<VkShaderStageFlags>(stageFlags),
                           reinterpret_cast<const VkSampler*>(pImmutableSampler)});
  }
  void setBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
  {
    m_bindings.setBindings(reinterpret_cast<const std::vector<VkDescriptorSetLayoutBinding>&>(bindings));
  }
  void setBindingFlags(uint32_t binding, vk::DescriptorBindingFlags bindingFlags)
  {
    m_bindings.setBindingFlags(binding, static_cast<VkDescriptorBindingFlags>(bindingFlags));
  }

  vk::WriteDescriptorSet makeWrite(uint32_t dstSetIdx, uint32_t dstBinding, const vk::DescriptorImageInfo* pImageInfo, uint32_t arrayElement = 0) const
  {
    return m_bindings.makeWrite(getSet(dstSetIdx), dstBinding, reinterpret_cast<const VkDescriptorImageInfo*>(pImageInfo), arrayElement);
  }
  vk::WriteDescriptorSet makeWrite(uint32_t                        dstSetIdx,
                                   uint32_t                        dstBinding,
                                   const vk::DescriptorBufferInfo* pBufferInfo,
                                   uint32_t                        arrayElement = 0) const
  {
    return m_bindings.makeWrite(getSet(dstSetIdx), dstBinding,
                                reinterpret_cast<const VkDescriptorBufferInfo*>(pBufferInfo), arrayElement);
  }
  vk::WriteDescriptorSet makeWrite(uint32_t dstSetIdx, uint32_t dstBinding, const vk::BufferView* pTexelBufferView, uint32_t arrayElement = 0) const
  {
    return m_bindings.makeWrite(getSet(dstSetIdx), dstBinding, reinterpret_cast<const VkBufferView*>(pTexelBufferView), arrayElement);
  }
#if VK_NV_ray_tracing
  vk::WriteDescriptorSet makeWrite(uint32_t                                             dstSetIdx,
                                   uint32_t                                             dstBinding,
                                   const vk::WriteDescriptorSetAccelerationStructureNV* pAccel,
                                   uint32_t                                             arrayElement = 0) const
  {
    return m_bindings.makeWrite(getSet(dstSetIdx), dstBinding,
                                reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureNV*>(pAccel), arrayElement);
  }
#endif
#if VK_KHR_acceleration_structure
  vk::WriteDescriptorSet makeWrite(uint32_t                                              dstSetIdx,
                                   uint32_t                                              dstBinding,
                                   const vk::WriteDescriptorSetAccelerationStructureKHR* pAccel,
                                   uint32_t                                              arrayElement = 0) const
  {
    return m_bindings.makeWrite(getSet(dstSetIdx), dstBinding,
                                reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(pAccel), arrayElement);
  }
#endif
  vk::WriteDescriptorSet makeWrite(uint32_t                                           dstSetIdx,
                                   uint32_t                                           dstBinding,
                                   const vk::WriteDescriptorSetInlineUniformBlockEXT* pInlineUniform,
                                   uint32_t                                           arrayElement = 0) const
  {
    return m_bindings.makeWrite(getSet(dstSetIdx), dstBinding,
                                reinterpret_cast<const VkWriteDescriptorSetInlineUniformBlockEXT*>(pInlineUniform), arrayElement);
  }
  vk::WriteDescriptorSet makeWriteArray(uint32_t dstSetIdx, uint32_t dstBinding, const vk::DescriptorImageInfo* pImageInfo) const
  {
    return m_bindings.makeWriteArray(getSet(dstSetIdx), dstBinding, reinterpret_cast<const VkDescriptorImageInfo*>(pImageInfo));
  }
  vk::WriteDescriptorSet makeWriteArray(uint32_t dstSetIdx, uint32_t dstBinding, const vk::DescriptorBufferInfo* pBufferInfo) const
  {
    return m_bindings.makeWriteArray(getSet(dstSetIdx), dstBinding, reinterpret_cast<const VkDescriptorBufferInfo*>(pBufferInfo));
  }
  vk::WriteDescriptorSet makeWriteArray(uint32_t dstSetIdx, uint32_t dstBinding, const vk::BufferView* pTexelBufferView) const
  {
    return m_bindings.makeWriteArray(getSet(dstSetIdx), dstBinding, reinterpret_cast<const VkBufferView*>(pTexelBufferView));
  }
#if VK_NV_ray_tracing
  vk::WriteDescriptorSet makeWriteArray(uint32_t dstSetIdx, uint32_t dstBinding, const vk::WriteDescriptorSetAccelerationStructureNV* pAccel) const
  {
    return m_bindings.makeWriteArray(getSet(dstSetIdx), dstBinding,
                                     reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureNV*>(pAccel));
  }
#endif
#if VK_KHR_acceleration_structure
  vk::WriteDescriptorSet makeWriteArray(uint32_t                                              dstSetIdx,
                                        uint32_t                                              dstBinding,
                                        const vk::WriteDescriptorSetAccelerationStructureKHR* pAccel) const
  {
    return m_bindings.makeWriteArray(getSet(dstSetIdx), dstBinding,
                                     reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(pAccel));
  }
#endif
#if VK_EXT_inline_uniform_block
  vk::WriteDescriptorSet makeWriteArray(uint32_t dstSetIdx, uint32_t dstBinding, const vk::WriteDescriptorSetInlineUniformBlockEXT* pInline) const
  {
    return m_bindings.makeWriteArray(getSet(dstSetIdx), dstBinding,
                                     reinterpret_cast<const VkWriteDescriptorSetInlineUniformBlockEXT*>(pInline));
  }
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

inline void cmdBarrierImageLayout(vk::CommandBuffer                cmdbuffer,
                                  vk::Image                        image,
                                  vk::ImageLayout                  oldImageLayout,
                                  vk::ImageLayout                  newImageLayout,
                                  const vk::ImageSubresourceRange& subresourceRange)
{
  nvvk::cmdBarrierImageLayout(static_cast<VkCommandBuffer>(cmdbuffer), static_cast<VkImage>(image),
                              static_cast<VkImageLayout>(oldImageLayout), static_cast<VkImageLayout>(newImageLayout),
                              static_cast<const VkImageSubresourceRange&>(subresourceRange));
}

inline void cmdBarrierImageLayout(vk::CommandBuffer    cmdbuffer,
                                  vk::Image            image,
                                  vk::ImageLayout      oldImageLayout,
                                  vk::ImageLayout      newImageLayout,
                                  vk::ImageAspectFlags aspectMask)
{
  nvvk::cmdBarrierImageLayout(static_cast<VkCommandBuffer>(cmdbuffer), static_cast<VkImage>(image),
                              static_cast<VkImageLayout>(oldImageLayout), static_cast<VkImageLayout>(newImageLayout),
                              static_cast<VkImageAspectFlags>(aspectMask));
}

inline void cmdBarrierImageLayout(vk::CommandBuffer cmdbuffer, vk::Image image, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout)
{
  cmdBarrierImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, vk::ImageAspectFlagBits::eColor);
}


inline vk::ImageCreateInfo makeImage2DCreateInfo(vk::Extent2D        size,
                                                 vk::Format          format  = vk::Format::eR8G8B8A8Unorm,
                                                 vk::ImageUsageFlags usage   = vk::ImageUsageFlagBits::eSampled,
                                                 bool                mipmaps = false)
{
  return nvvk::makeImage2DCreateInfo(static_cast<VkExtent2D>(size), static_cast<VkFormat>(format),
                                     static_cast<VkImageUsageFlags>(usage), mipmaps);
}

inline vk::ImageViewCreateInfo makeImage2DViewCreateInfo(vk::Image            image,
                                                         vk::Format           format,
                                                         vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
                                                         uint32_t             levels      = VK_REMAINING_MIP_LEVELS,
                                                         const void*          pNextImageView = nullptr)
{
  return nvvk::makeImage2DViewCreateInfo(static_cast<VkImage>(image), static_cast<VkFormat>(format),
                                         static_cast<VkImageAspectFlags>(aspectFlags), levels, pNextImageView);
}

inline vk::ImageViewCreateInfo makeImageViewCreateInfo(vk::Image image, const vk::ImageCreateInfo& imageInfo, bool isCube = false)
{
  return nvvk::makeImageViewCreateInfo(static_cast<VkImage>(image), static_cast<VkImageCreateInfo>(imageInfo), isCube);
}

inline vk::ImageViewCreateInfo makeImageViewCreateInfo(VkImage image, const vk::ImageCreateInfo& imageInfo, bool isCube = false)
{
  return nvvk::makeImageViewCreateInfo(image, static_cast<VkImageCreateInfo>(imageInfo), isCube);
}

inline vk::ImageCreateInfo makeImageCubeCreateInfo(const vk::Extent2D& size,
                                                   vk::Format          format,
                                                   vk::ImageUsageFlags usage   = vk::ImageUsageFlagBits::eSampled,
                                                   bool                mipmaps = false)
{
  return nvvk::makeImageCubeCreateInfo(static_cast<VkExtent2D>(size), static_cast<VkFormat>(format),
                                       static_cast<VkImageUsageFlags>(usage), mipmaps);
}

inline void cmdGenerateMipmaps(vk::CommandBuffer   cmdBuf,
                               vk::Image           image,
                               vk::Format          imageFormat,
                               const vk::Extent2D& size,
                               uint32_t            levelCount,
                               uint32_t            layerCount    = 1,
                               vk::ImageLayout     currentLayout = vk::ImageLayout::eShaderReadOnlyOptimal)
{
  nvvk::cmdGenerateMipmaps(static_cast<VkCommandBuffer>(cmdBuf), static_cast<VkImage>(image), static_cast<VkFormat>(imageFormat),
                           static_cast<VkExtent2D>(size), levelCount, layerCount, static_cast<VkImageLayout>(currentLayout));
}

inline vk::ImageCreateInfo makeImage3DCreateInfo(const vk::Extent3D& size,
                                                 vk::Format          format,
                                                 vk::ImageUsageFlags usage   = vk::ImageUsageFlagBits::eSampled,
                                                 bool                mipmaps = false)
{
  return nvvk::makeImage3DCreateInfo(static_cast<VkExtent3D>(size), static_cast<VkFormat>(format),
                                     static_cast<VkImageUsageFlags>(usage), mipmaps);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct GraphicsPipelineState
{
  // Initialize the state to common values: triangle list topology, depth test enabled,
  // dynamic viewport and scissor, one render target, blending disabled
  GraphicsPipelineState()
  {
    rasterizationState.flags                   = {};
    rasterizationState.depthClampEnable        = {};
    rasterizationState.rasterizerDiscardEnable = {};
    setValue(rasterizationState.polygonMode, VK_POLYGON_MODE_FILL);
    setValue(rasterizationState.cullMode, VK_CULL_MODE_BACK_BIT);
    setValue(rasterizationState.frontFace, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    rasterizationState.depthBiasEnable         = {};
    rasterizationState.depthBiasConstantFactor = {};
    rasterizationState.depthBiasClamp          = {};
    rasterizationState.depthBiasSlopeFactor    = {};
    rasterizationState.lineWidth               = 1.f;

    inputAssemblyState.flags = {};
    setValue(inputAssemblyState.topology, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    inputAssemblyState.primitiveRestartEnable = {};


    colorBlendState.flags         = {};
    colorBlendState.logicOpEnable = {};
    setValue(colorBlendState.logicOp, VK_LOGIC_OP_CLEAR);
    colorBlendState.attachmentCount = {};
    colorBlendState.pAttachments    = {};
    for(int i = 0; i < 4; i++)
    {
      colorBlendState.blendConstants[i] = 0.f;
    }


    dynamicState.flags             = {};
    dynamicState.dynamicStateCount = {};
    dynamicState.pDynamicStates    = {};


    vertexInputState.flags                           = {};
    vertexInputState.vertexBindingDescriptionCount   = {};
    vertexInputState.pVertexBindingDescriptions      = {};
    vertexInputState.vertexAttributeDescriptionCount = {};
    vertexInputState.pVertexAttributeDescriptions    = {};


    viewportState.flags         = {};
    viewportState.viewportCount = {};
    viewportState.pViewports    = {};
    viewportState.scissorCount  = {};
    viewportState.pScissors     = {};


    depthStencilState.flags            = {};
    depthStencilState.depthTestEnable  = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    setValue(depthStencilState.depthCompareOp, VK_COMPARE_OP_LESS_OR_EQUAL);
    depthStencilState.depthBoundsTestEnable = {};
    depthStencilState.stencilTestEnable     = {};
    setValue(depthStencilState.front, VkStencilOpState());
    setValue(depthStencilState.back, VkStencilOpState());
    depthStencilState.minDepthBounds = {};
    depthStencilState.maxDepthBounds = {};

    setValue(multisampleState.rasterizationSamples, VK_SAMPLE_COUNT_1_BIT);
  }

  // Attach the pointer values of the structures to the internal arrays
  void update()
  {
    colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
    colorBlendState.pAttachments    = blendAttachmentStates.data();

    dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
    dynamicState.pDynamicStates    = dynamicStateEnables.data();

    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputState.pVertexBindingDescriptions      = bindingDescriptions.data();
    vertexInputState.pVertexAttributeDescriptions    = attributeDescriptions.data();

    if(viewports.empty())
    {
      viewportState.viewportCount = 1;
      viewportState.pViewports    = nullptr;
    }
    else
    {
      viewportState.viewportCount = (uint32_t)viewports.size();
      viewportState.pViewports    = viewports.data();
    }

    if(scissors.empty())
    {
      viewportState.scissorCount = 1;
      viewportState.pScissors    = nullptr;
    }
    else
    {
      viewportState.scissorCount = (uint32_t)scissors.size();
      viewportState.pScissors    = scissors.data();
    }
  }

  static inline vk::PipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(
      vk::ColorComponentFlags colorWriteMask_ = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                                | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
      vk::Bool32      blendEnable_         = 0,
      vk::BlendFactor srcColorBlendFactor_ = vk::BlendFactor::eZero,
      vk::BlendFactor dstColorBlendFactor_ = vk::BlendFactor::eZero,
      vk::BlendOp     colorBlendOp_        = vk::BlendOp::eAdd,
      vk::BlendFactor srcAlphaBlendFactor_ = vk::BlendFactor::eZero,
      vk::BlendFactor dstAlphaBlendFactor_ = vk::BlendFactor::eZero,
      vk::BlendOp     alphaBlendOp_        = vk::BlendOp::eAdd)
  {
    vk::PipelineColorBlendAttachmentState res;

    res.blendEnable         = blendEnable_;
    res.srcColorBlendFactor = srcColorBlendFactor_;
    res.dstColorBlendFactor = dstColorBlendFactor_;
    res.colorBlendOp        = colorBlendOp_;
    res.srcAlphaBlendFactor = srcAlphaBlendFactor_;
    res.dstAlphaBlendFactor = dstAlphaBlendFactor_;
    res.alphaBlendOp        = alphaBlendOp_;
    res.colorWriteMask      = colorWriteMask_;
    return res;
  }

  static inline VkVertexInputBindingDescription makeVertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX)
  {
    VkVertexInputBindingDescription vertexBinding;
    vertexBinding.binding   = binding;
    vertexBinding.inputRate = rate;
    vertexBinding.stride    = stride;
    return vertexBinding;
  }

  static inline VkVertexInputAttributeDescription makeVertexInputAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
  {
    VkVertexInputAttributeDescription attrib;
    attrib.binding  = binding;
    attrib.location = location;
    attrib.format   = format;
    attrib.offset   = offset;
    return attrib;
  }


  void clearBlendAttachmentStates() { blendAttachmentStates.clear(); }
  void setBlendAttachmentCount(uint32_t attachmentCount) { blendAttachmentStates.resize(attachmentCount); }

  void setBlendAttachmentState(uint32_t attachment, const vk::PipelineColorBlendAttachmentState& blendState)
  {
    assert(attachment < blendAttachmentStates.size());
    if(attachment <= blendAttachmentStates.size())
    {
      blendAttachmentStates[attachment] = blendState;
    }
  }

  uint32_t addBlendAttachmentState(const vk::PipelineColorBlendAttachmentState& blendState)
  {
    blendAttachmentStates.push_back(blendState);
    return (uint32_t)(blendAttachmentStates.size() - 1);
  }

  void clearDynamicStateEnables() { dynamicStateEnables.clear(); }
  void setDynamicStateEnablesCount(uint32_t dynamicStateCount) { dynamicStateEnables.resize(dynamicStateCount); }

  void setDynamicStateEnable(uint32_t state, vk::DynamicState dynamicState)
  {
    assert(state < dynamicStateEnables.size());
    if(state <= dynamicStateEnables.size())
    {
      dynamicStateEnables[state] = dynamicState;
    }
  }

  uint32_t addDynamicStateEnable(vk::DynamicState dynamicState)
  {
    dynamicStateEnables.push_back(dynamicState);
    return (uint32_t)(dynamicStateEnables.size() - 1);
  }


  void clearBindingDescriptions() { bindingDescriptions.clear(); }
  void setBindingDescriptionsCount(uint32_t bindingDescriptionCount)
  {
    bindingDescriptions.resize(bindingDescriptionCount);
  }
  void setBindingDescription(uint32_t binding, VkVertexInputBindingDescription bindingDescription)
  {
    assert(binding < bindingDescriptions.size());
    if(binding <= bindingDescriptions.size())
    {
      bindingDescriptions[binding] = bindingDescription;
    }
  }

  uint32_t addBindingDescription(const vk::VertexInputBindingDescription& bindingDescription)
  {
    bindingDescriptions.push_back(bindingDescription);
    return (uint32_t)(bindingDescriptions.size() - 1);
  }

  void addBindingDescriptions(const std::vector<vk::VertexInputBindingDescription>& bindingDescriptions_)
  {
    bindingDescriptions.insert(bindingDescriptions.end(), bindingDescriptions_.begin(), bindingDescriptions_.end());
  }

  void clearAttributeDescriptions() { attributeDescriptions.clear(); }
  void setAttributeDescriptionsCount(uint32_t attributeDescriptionCount)
  {
    attributeDescriptions.resize(attributeDescriptionCount);
  }

  void setAttributeDescription(uint32_t attribute, const vk::VertexInputAttributeDescription& attributeDescription)
  {
    assert(attribute < attributeDescriptions.size());
    if(attribute <= attributeDescriptions.size())
    {
      attributeDescriptions[attribute] = attributeDescription;
    }
  }


  uint32_t addAttributeDescription(const vk::VertexInputAttributeDescription& attributeDescription)
  {
    attributeDescriptions.push_back(attributeDescription);
    return (uint32_t)(attributeDescriptions.size() - 1);
  }

  void addAttributeDescriptions(const std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions_)
  {
    attributeDescriptions.insert(attributeDescriptions.end(), attributeDescriptions_.begin(), attributeDescriptions_.end());
  }


  void clearViewports() { viewports.clear(); }
  void setViewportsCount(uint32_t viewportCount) { viewports.resize(viewportCount); }
  void setViewport(uint32_t attribute, VkViewport viewport)
  {
    assert(attribute < viewports.size());
    if(attribute <= viewports.size())
    {
      viewports[attribute] = viewport;
    }
  }
  uint32_t addViewport(VkViewport viewport)
  {
    viewports.push_back(viewport);
    return (uint32_t)(viewports.size() - 1);
  }


  void clearScissors() { scissors.clear(); }
  void setScissorsCount(uint32_t scissorCount) { scissors.resize(scissorCount); }
  void setScissor(uint32_t attribute, VkRect2D scissor)
  {
    assert(attribute < scissors.size());
    if(attribute <= scissors.size())
    {
      scissors[attribute] = scissor;
    }
  }
  uint32_t addScissor(VkRect2D scissor)
  {
    scissors.push_back(scissor);
    return (uint32_t)(scissors.size() - 1);
  }


  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
  vk::PipelineRasterizationStateCreateInfo rasterizationState;
  vk::PipelineMultisampleStateCreateInfo   multisampleState;
  vk::PipelineDepthStencilStateCreateInfo  depthStencilState;
  vk::PipelineViewportStateCreateInfo      viewportState;
  vk::PipelineDynamicStateCreateInfo       dynamicState;
  vk::PipelineColorBlendStateCreateInfo    colorBlendState;
  vk::PipelineVertexInputStateCreateInfo   vertexInputState;

private:
  std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates{makePipelineColorBlendAttachmentState()};
  std::vector<vk::DynamicState> dynamicStateEnables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

  std::vector<vk::VertexInputBindingDescription>   bindingDescriptions;
  std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

  std::vector<vk::Viewport> viewports;
  std::vector<vk::Rect2D>   scissors;

  // Helper to set objects for either C and C++
  template <class T, class U>
  void setValue(T& target, const U& val)
  {
    target = (T)(val);
  }
};

struct GraphicsPipelineGenerator
{
public:
  GraphicsPipelineGenerator(GraphicsPipelineState& pipelineState_)
      : pipelineState(pipelineState_)
  {
    init();
  }

  GraphicsPipelineGenerator(const GraphicsPipelineGenerator& src)
      : createInfo(src.createInfo)
      , device(src.device)
      , pipelineCache(src.pipelineCache)
      , pipelineState(src.pipelineState)
  {
    init();
  }

  GraphicsPipelineGenerator(VkDevice device_, const VkPipelineLayout& layout, const VkRenderPass& renderPass, GraphicsPipelineState& pipelineState_)
      : device(device_)
      , pipelineState(pipelineState_)
  {
    createInfo.layout     = layout;
    createInfo.renderPass = renderPass;
    init();
  }

  // For VK_KHR_dynamic_rendering
  GraphicsPipelineGenerator(VkDevice                               device_,
                            const VkPipelineLayout&                layout,
                            const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                            GraphicsPipelineState&                 pipelineState_)
      : device(device_)
      , pipelineState(pipelineState_)
  {
    createInfo.layout = layout;
    setPipelineRenderingCreateInfo(pipelineRenderingCreateInfo);
    init();
  }

  const GraphicsPipelineGenerator& operator=(const GraphicsPipelineGenerator& src)
  {
    device        = src.device;
    pipelineState = src.pipelineState;
    createInfo    = src.createInfo;
    pipelineCache = src.pipelineCache;

    init();
    return *this;
  }

  void setDevice(VkDevice device_) { device = device_; }

  void setRenderPass(VkRenderPass renderPass)
  {
    createInfo.renderPass = renderPass;
    createInfo.pNext      = nullptr;
  }

  void setPipelineRenderingCreateInfo(const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo)
  {
    // Deep copy
    assert(pipelineRenderingCreateInfo.pNext == nullptr);  // Update deep copy if needed.
    dynamicRenderingInfo = pipelineRenderingCreateInfo;
    if(dynamicRenderingInfo.colorAttachmentCount != 0)
    {
      dynamicRenderingColorFormats.assign(dynamicRenderingInfo.pColorAttachmentFormats,
                                          dynamicRenderingInfo.pColorAttachmentFormats + dynamicRenderingInfo.colorAttachmentCount);
      dynamicRenderingInfo.pColorAttachmentFormats = dynamicRenderingColorFormats.data();
    }

    // Set VkGraphicsPipelineCreateInfo::pNext to point to deep copy of extension struct.
    // NB: Will have to change if more than 1 extension struct needs to be supported.
    createInfo.pNext = &dynamicRenderingInfo;
  }

  void setLayout(VkPipelineLayout layout) { createInfo.layout = layout; }

  ~GraphicsPipelineGenerator() { destroyShaderModules(); }

  vk::PipelineShaderStageCreateInfo& addShader(const std::string& code, vk::ShaderStageFlagBits stage, const char* entryPoint = "main")
  {
    std::vector<char> v;
    std::copy(code.begin(), code.end(), std::back_inserter(v));
    return addShader(v, stage, entryPoint);
  }

  template <typename T>
  vk::PipelineShaderStageCreateInfo& addShader(const std::vector<T>& code, vk::ShaderStageFlagBits stage, const char* entryPoint = "main")
  {
    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = sizeof(T) * code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    temporaryModules.push_back(shaderModule);

    return addShader(shaderModule, stage, entryPoint);
  }

  vk::PipelineShaderStageCreateInfo& addShader(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits stage, const char* entryPoint = "main")
  {
    VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    shaderStage.stage  = (VkShaderStageFlagBits)stage;
    shaderStage.module = shaderModule;
    shaderStage.pName  = entryPoint;

    shaderStages.push_back(shaderStage);
    return shaderStages.back();
  }

  void clearShaders()
  {
    shaderStages.clear();
    destroyShaderModules();
  }

  VkShaderModule getShaderModule(size_t index) const
  {
    if(index < shaderStages.size())
      return shaderStages[index].module;
    return VK_NULL_HANDLE;
  }

  VkPipeline createPipeline(const VkPipelineCache& cache)
  {
    update();
    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device, cache, 1, (VkGraphicsPipelineCreateInfo*)&createInfo, nullptr, &pipeline);
    return pipeline;
  }

  VkPipeline createPipeline() { return createPipeline(pipelineCache); }

  void destroyShaderModules()
  {
    for(const auto& shaderModule : temporaryModules)
    {
      vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    temporaryModules.clear();
  }
  void update()
  {
    createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    createInfo.pStages    = shaderStages.data();
    pipelineState.update();
  }

  vk::GraphicsPipelineCreateInfo createInfo;

private:
  vk::Device        device;
  vk::PipelineCache pipelineCache;

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
  std::vector<vk::ShaderModule>                  temporaryModules;
  std::vector<vk::Format>                        dynamicRenderingColorFormats;

  GraphicsPipelineState&          pipelineState;
  vk::PipelineRenderingCreateInfo dynamicRenderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};


  void init()
  {
    createInfo.pRasterizationState = &pipelineState.rasterizationState;
    createInfo.pInputAssemblyState = &pipelineState.inputAssemblyState;
    createInfo.pColorBlendState    = &pipelineState.colorBlendState;
    createInfo.pMultisampleState   = &pipelineState.multisampleState;
    createInfo.pViewportState      = &pipelineState.viewportState;
    createInfo.pDepthStencilState  = &pipelineState.depthStencilState;
    createInfo.pDynamicState       = &pipelineState.dynamicState;
    createInfo.pVertexInputState   = &pipelineState.vertexInputState;
  }

  // Helper to set objects for either C and C++
  template <class T, class U>
  void setValue(T& target, const U& val)
  {
    target = (T)(val);
  }
};


//--------------------------------------------------------------------------------------------------
/**
\class nvvk::GraphicsPipelineGeneratorCombined

In some cases the application may have each state associated to a single pipeline. For convenience,
    nvvk::GraphicsPipelineGeneratorCombined combines both the state and generator into a single object.

    Example of usage :
\code{.cpp}
                        nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
pipelineGenerator.depthStencilState.setDepthTestEnable(true);
pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineGenerator.addBindingDescription({0, sizeof(Vertex)});
pipelineGenerator.addAttributeDescriptions ({
                                            {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
                                            {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
                                            {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});

pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
\endcode
        */


struct GraphicsPipelineGeneratorCombined : public GraphicsPipelineState, public GraphicsPipelineGenerator
{
  GraphicsPipelineGeneratorCombined(VkDevice device_, const VkPipelineLayout& layout, const VkRenderPass& renderPass)
      : GraphicsPipelineState()
      , GraphicsPipelineGenerator(device_, layout, renderPass, *this)
  {
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


// Ray tracing BLAS and TLAS builder
class RaytracingBuilderKHR : public nvvk::RaytracingBuilderKHR
{
public:
  void buildBlas(const std::vector<RaytracingBuilderKHR::BlasInput>& blas_, vk::BuildAccelerationStructureFlagsKHR flags)
  {
    nvvk::RaytracingBuilderKHR::buildBlas(blas_, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags));
  }

  void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
                 vk::BuildAccelerationStructureFlagsKHR                 flags,
                 bool                                                   update = false)
  {
    nvvk::RaytracingBuilderKHR::buildTlas(instances, static_cast<VkBuildAccelerationStructureFlagsKHR>(flags), update);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


class RaytracingBuilderNV : public nvvk::RaytracingBuilderNV
{
public:
  void buildBlas(const std::vector<std::vector<VkGeometryNV>>& geoms,
                 vk::BuildAccelerationStructureFlagsNV flags = vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace)
  {
    nvvk::RaytracingBuilderNV::buildBlas(geoms, static_cast<VkBuildAccelerationStructureFlagsNV>(flags));
  }

  void buildTlas(const std::vector<Instance>&          instances,
                 vk::BuildAccelerationStructureFlagsNV flags = vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace)
  {
    nvvk::RaytracingBuilderNV::buildTlas(instances, static_cast<VkBuildAccelerationStructureFlagsNV>(flags));
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

inline vk::Format findDepthFormat(vk::PhysicalDevice physicalDevice)
{
  return (vk::Format)nvvk::findDepthFormat((VkPhysicalDevice)physicalDevice);
}

inline vk::Format findDepthStencilFormat(vk::PhysicalDevice physicalDevice)
{
  return (vk::Format)nvvk::findDepthStencilFormat((VkPhysicalDevice)physicalDevice);
}

inline vk::Format findSupportedFormat(vk::PhysicalDevice             physicalDevice,
                                      const std::vector<vk::Format>& candidates,
                                      vk::ImageTiling                tiling,
                                      vk::FormatFeatureFlags         features)
{
  return (vk::Format)nvvk::findSupportedFormat((VkPhysicalDevice)physicalDevice, (const std::vector<VkFormat>&)candidates,
                                               (VkImageTiling)tiling, (VkFormatFeatureFlags)features);
}
inline vk::RenderPass createRenderPass(vk::Device                     device,
                                       const std::vector<vk::Format>& colorAttachmentFormats,
                                       vk::Format                     depthAttachmentFormat,
                                       uint32_t                       subpassCount  = 1,
                                       bool                           clearColor    = true,
                                       bool                           clearDepth    = true,
                                       vk::ImageLayout                initialLayout = vk::ImageLayout::eUndefined,
                                       vk::ImageLayout                finalLayout   = vk::ImageLayout::ePresentSrcKHR)
{
  return (vk::RenderPass)nvvk::createRenderPass((VkDevice)device, (const std::vector<VkFormat>&)colorAttachmentFormats,
                                                (VkFormat)depthAttachmentFormat, subpassCount, clearColor, clearDepth,
                                                (VkImageLayout)initialLayout, (VkImageLayout)finalLayout);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class ResourceAllocator : public nvvk::ResourceAllocator
{
public:
  ResourceAllocator(ResourceAllocator const&) = delete;
  ResourceAllocator& operator=(ResourceAllocator const&) = delete;

  ResourceAllocator() = default;
  ResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::MemAllocator* memAllocator, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE)
      : nvvk::ResourceAllocator(device, physicalDevice, memAllocator, stagingBlockSize)
  {
  }

  // All staging buffers must be cleared before
  virtual ~ResourceAllocator(){};

  inline nvvk::Texture createTexture(const vk::CommandBuffer&     cmdBuf,
                                     size_t                       size_,
                                     const void*                  data_,
                                     const vk::ImageCreateInfo&   info_,
                                     const vk::SamplerCreateInfo& samplerCreateInfo,
                                     const vk::ImageLayout&       layout_ = vk::ImageLayout::eShaderReadOnlyOptimal,
                                     bool                         isCube  = false)
  {
    return nvvk::ResourceAllocator::createTexture(static_cast<VkCommandBuffer>(cmdBuf), size_, data_,
                                                  static_cast<VkImageCreateInfo>(info_),
                                                  static_cast<VkSamplerCreateInfo>(samplerCreateInfo),
                                                  static_cast<VkImageLayout>(layout_), isCube);
  }

  nvvk::Texture createTexture(const nvvk::Image& image, const vk::ImageViewCreateInfo& imageViewCreateInfo)
  {
    return nvvk::ResourceAllocator::createTexture(image, static_cast<const VkImageViewCreateInfo&>(imageViewCreateInfo));
  }
  nvvk::Texture createTexture(const nvvk::Image&             image,
                              const vk::ImageViewCreateInfo& imageViewCreateInfo,
                              const vk::SamplerCreateInfo&   samplerCreateInfo)
  {
    return nvvk::ResourceAllocator::createTexture(image, static_cast<const VkImageViewCreateInfo&>(imageViewCreateInfo),
                                                  static_cast<const VkSamplerCreateInfo&>(samplerCreateInfo));
  }


  nvvk::Buffer createBuffer(const vk::BufferCreateInfo&   info_,
                            const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return nvvk::ResourceAllocator::createBuffer(static_cast<VkBufferCreateInfo>(info_),
                                                 static_cast<VkMemoryPropertyFlags>(memUsage_));
  }

  nvvk::Buffer createBuffer(vk::DeviceSize                size_,
                            vk::BufferUsageFlags          usage_,
                            const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return nvvk::ResourceAllocator::createBuffer(static_cast<VkDeviceSize>(size_), static_cast<VkBufferUsageFlags>(usage_),
                                                 static_cast<VkMemoryPropertyFlags>(memUsage_));
  }

  nvvk::Buffer createBuffer(const vk::CommandBuffer& cmdBuf,
                            vk::DeviceSize           size_,
                            const void*              data_,
                            vk::BufferUsageFlags     usage_,
                            vk::MemoryPropertyFlags  memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return nvvk::ResourceAllocator::createBuffer(static_cast<VkCommandBuffer>(cmdBuf), static_cast<VkDeviceSize>(size_),
                                                 data_, static_cast<VkBufferUsageFlags>(usage_),
                                                 static_cast<VkMemoryPropertyFlags>(memUsage_));
  }

  template <typename T>
  nvvk::Buffer createBuffer(const vk::CommandBuffer&    cmdBuff,
                            const std::vector<T>&       data_,
                            const vk::BufferUsageFlags& usage_,
                            vk::MemoryPropertyFlags     memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return createBuffer(cmdBuff, sizeof(T) * data_.size(), data_.data(), usage_, memUsage_);
  }

  nvvk::Image createImage(const vk::ImageCreateInfo&    info_,
                          const vk::MemoryPropertyFlags memUsage_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
  {
    return nvvk::ResourceAllocator::createImage(static_cast<const VkImageCreateInfo&>(info_),
                                                static_cast<VkMemoryPropertyFlags>(memUsage_));
  }

  nvvk::Image createImage(const vk::CommandBuffer&   cmdBuff,
                          size_t                     size_,
                          const void*                data_,
                          const vk::ImageCreateInfo& info_,
                          const vk::ImageLayout&     layout_ = vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    return nvvk::ResourceAllocator::createImage(static_cast<VkCommandBuffer>(cmdBuff), size_, data_,
                                                static_cast<const VkImageCreateInfo&>(info_), static_cast<VkImageLayout>(layout_));
  }

  nvvk::AccelNV createAcceleration(vk::AccelerationStructureCreateInfoNV& accel_)
  {
    return nvvk::ResourceAllocator::createAcceleration(static_cast<VkAccelerationStructureCreateInfoNV&>(accel_));
  }

  nvvk::AccelKHR createAcceleration(vk::AccelerationStructureCreateInfoKHR& accel_)
  {
    return nvvk::ResourceAllocator::createAcceleration(static_cast<VkAccelerationStructureCreateInfoKHR&>(accel_));
  }
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 \class nvvk::ResourceAllocatorDma
 nvvk::ResourceAllocatorDMA is a convencience class owning a nvvk::DMAMemoryAllocator and nvvk::DeviceMemoryAllocator object
*/
class ResourceAllocatorDma : public ResourceAllocator
{
public:
  ResourceAllocatorDma() = default;
  ResourceAllocatorDma(VkDevice         device,
                       VkPhysicalDevice physicalDevice,
                       VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
                       VkDeviceSize     memBlockSize     = 0);
  virtual ~ResourceAllocatorDma();

  void init(VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkDeviceSize     memBlockSize     = 0);
  // Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
  void init(VkInstance,
            VkDevice         device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize     stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE,
            VkDeviceSize     memBlockSize     = 0);

  void deinit();

  nvvk::DeviceMemoryAllocator*       getDMA() { return m_dma.get(); }
  const nvvk::DeviceMemoryAllocator* getDMA() const { return m_dma.get(); }

protected:
  std::unique_ptr<nvvk::DeviceMemoryAllocator> m_dma;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 \class nvvk::ResourceAllocatorDedicated
 \brief nvvk::ResourceAllocatorDedicated is a convencience class automatically creating and owning a DedicatedMemoryAllocator object
 */
class ResourceAllocatorDedicated : public ResourceAllocator
{
public:
  ResourceAllocatorDedicated() = default;
  ResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  virtual ~ResourceAllocatorDedicated();

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  // Provided such that ResourceAllocatorDedicated, ResourceAllocatorDma and ResourceAllocatorVma all have the same interface
  void init(VkInstance, VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

  void deinit();

protected:
  std::unique_ptr<nvvk::MemAllocator> m_memAlloc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 #class nvvk::ExportResourceAllocator

ExportResourceAllocator specializes the object allocation process such that resulting memory allocations are
        exportable and buffers and images can be bound to external memory.
            */
class ExportResourceAllocator : public ResourceAllocator
{
public:
  ExportResourceAllocator() = default;
  ExportResourceAllocator(VkDevice            device,
                          VkPhysicalDevice    physicalDevice,
                          nvvk::MemAllocator* memAlloc,
                          VkDeviceSize        stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);

protected:
  virtual nvvk::MemHandle AllocateMemory(const nvvk::MemAllocateInfo& allocateInfo) override;
  virtual void            CreateBufferEx(const VkBufferCreateInfo& info_, VkBuffer* buffer) override;
  virtual void            CreateImageEx(const VkImageCreateInfo& info_, VkImage* image) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 \class nvvk::ExportResourceAllocatorDedicated
 nvvk::ExportResourceAllocatorDedicated is a resource allocator that is using DedicatedMemoryAllocator to allocate memory
 and at the same time it'll make all allocations exportable.
*/
class ExportResourceAllocatorDedicated : public ExportResourceAllocator
{
public:
  ExportResourceAllocatorDedicated() = default;
  ExportResourceAllocatorDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  virtual ~ExportResourceAllocatorDedicated() override;

  void init(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize stagingBlockSize = NVVK_DEFAULT_STAGING_BLOCKSIZE);
  void deinit();

protected:
  std::unique_ptr<nvvk::MemAllocator> m_memAlloc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 \class nvvk::ExplicitDeviceMaskResourceAllocator
 nvvk::ExplicitDeviceMaskResourceAllocator is a resource allocator that will inject a specific devicemask into each
 allocation, making the created allocations and objects available to only the devices in the mask.
*/
class ExplicitDeviceMaskResourceAllocator : public ResourceAllocator
{
public:
  ExplicitDeviceMaskResourceAllocator() = default;
  ExplicitDeviceMaskResourceAllocator(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::MemAllocator* memAlloc, uint32_t deviceMask);

  void init(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::MemAllocator* memAlloc, uint32_t deviceMask);

protected:
  virtual nvvk::MemHandle AllocateMemory(const nvvk::MemAllocateInfo& allocateInfo) override;

  uint32_t m_deviceMask;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline vk::SamplerCreateInfo makeSamplerCreateInfo(vk::Filter             magFilter = vk::Filter::eLinear,
                                                   vk::Filter             minFilter = vk::Filter::eLinear,
                                                   vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eClampToEdge,
                                                   vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eClampToEdge,
                                                   vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eClampToEdge,
                                                   vk::Bool32             anisotropyEnable = VK_FALSE,
                                                   float                  maxAnisotropy    = 16,
                                                   vk::SamplerMipmapMode mipmapMode    = vk::SamplerMipmapMode::eLinear,
                                                   float                 minLod        = 0.0f,
                                                   float                 maxLod        = FLT_MAX,
                                                   float                 mipLodBias    = 0.0f,
                                                   vk::Bool32            compareEnable = VK_FALSE,
                                                   vk::CompareOp         compareOp     = vk::CompareOp::eAlways,
                                                   vk::BorderColor       borderColor = vk::BorderColor::eIntOpaqueBlack,
                                                   vk::Bool32            unnormalizedCoordinates = VK_FALSE)
{
  return nvvk::makeSamplerCreateInfo(
      static_cast<VkFilter>(magFilter), static_cast<VkFilter>(minFilter), static_cast<VkSamplerAddressMode>(addressModeU),
      static_cast<VkSamplerAddressMode>(addressModeV), static_cast<VkSamplerAddressMode>(addressModeW),
      static_cast<VkBool32>(anisotropyEnable), maxAnisotropy, static_cast<VkSamplerMipmapMode>(mipmapMode), minLod,
      maxLod, mipLodBias, compareEnable, static_cast<VkCompareOp>(compareOp), static_cast<VkBorderColor>(borderColor),
      static_cast<VkBool32>(unnormalizedCoordinates));
}

}  // namespace nvvkpp
