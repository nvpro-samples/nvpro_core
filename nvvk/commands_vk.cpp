/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include <algorithm>
#include <platform.h>


#include "commands_vk.hpp"
#include "error_vk.hpp"


namespace nvvk {
uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask, VkPipelineStageFlags supportedShaderBits)
{
  static const uint32_t accessPipes[] = {
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
    VK_ACCESS_INDEX_READ_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    VK_ACCESS_UNIFORM_READ_BIT,
    supportedShaderBits,
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_ACCESS_SHADER_READ_BIT,
    supportedShaderBits,
    VK_ACCESS_SHADER_WRITE_BIT,
    supportedShaderBits,
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
#if VK_NV_device_generated_commands
    VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV,
    VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
    VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV,
    VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
#endif
#if VK_NV_ray_tracing
    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | supportedShaderBits | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
#endif
  };
  if(!accessMask)
  {
    return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }

  uint32_t pipes = 0;

  for(uint32_t i = 0; i < NV_ARRAY_SIZE(accessPipes); i += 2)
  {
    if(accessPipes[i] & accessMask)
    {
      pipes |= accessPipes[i + 1];
    }
  }
  assert(pipes != 0);

  return pipes;
}

void cmdBegin(VkCommandBuffer cmd, VkCommandBufferUsageFlags flags)
{
  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = flags;

  VkResult res = vkBeginCommandBuffer(cmd, &beginInfo);
  assert(res == VK_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

void CommandPool::init(VkDevice device, uint32_t familyIndex, VkCommandPoolCreateFlags flags, VkQueue defaultQueue)
{
  assert(!m_device);
  m_device                     = device;
  VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  info.flags                   = flags;
  info.queueFamilyIndex        = familyIndex;
  vkCreateCommandPool(m_device, &info, nullptr, &m_commandPool);
  if(defaultQueue)
  {
    m_queue = defaultQueue;
  }
  else
  {
    vkGetDeviceQueue(device, familyIndex, 0, &m_queue);
  }
}

void CommandPool::deinit()
{
  if(m_commandPool)
  {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
  }
  m_device = VK_NULL_HANDLE;
}

VkCommandBuffer CommandPool::createCommandBuffer(VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
                                                 bool                 begin,
                                                 VkCommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/,
                                                 const VkCommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/)
{
  VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.level                       = level;
  allocInfo.commandPool                 = m_commandPool;
  allocInfo.commandBufferCount          = 1;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(m_device, &allocInfo, &cmd);

  if(begin)
  {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = flags;
    beginInfo.pInheritanceInfo         = pInheritanceInfo;

    vkBeginCommandBuffer(cmd, &beginInfo);
  }

  return cmd;
}

void CommandPool::destroy(size_t count, const VkCommandBuffer* cmds)
{
  vkFreeCommandBuffers(m_device, m_commandPool, (uint32_t)count, cmds);
}


void CommandPool::submitAndWait(size_t count, const VkCommandBuffer* cmds, VkQueue queue)
{
  submit(count, cmds, queue);
  VkResult result = vkQueueWaitIdle(queue);
  if(nvvk::checkResult(result, __FILE__, __LINE__))
  {
    exit(-1);
  }
  vkFreeCommandBuffers(m_device, m_commandPool, (uint32_t)count, cmds);
}

void CommandPool::submit(size_t count, const VkCommandBuffer* cmds, VkQueue queue, VkFence fence)
{
  for(size_t i = 0; i < count; i++)
  {
    vkEndCommandBuffer(cmds[i]);
  }

  VkSubmitInfo submit       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.pCommandBuffers    = cmds;
  submit.commandBufferCount = (uint32_t)count;
  vkQueueSubmit(queue, 1, &submit, fence);
}

void CommandPool::submit(size_t count, const VkCommandBuffer* cmds, VkFence fence)
{
  submit(count, cmds, m_queue, fence);
}

void CommandPool::submit(const std::vector<VkCommandBuffer>& cmds, VkFence fence)
{
  submit(cmds.size(), cmds.data(), m_queue, fence);
}

//////////////////////////////////////////////////////////////////////////

void RingFences::init(VkDevice device, uint32_t ringSize)
{
  assert(!m_device);
  m_device     = device;
  m_cycleIndex = 0;
  m_cycleSize  = ringSize;

  m_fences.resize(ringSize);
  for(uint32_t i = 0; i < m_cycleSize; i++)
  {
    VkFenceCreateInfo info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    info.flags             = 0;
    NVVK_CHECK(vkCreateFence(device, &info, nullptr, &m_fences[i].fence));
    m_fences[i].active = false;
  }
}

void RingFences::deinit()
{
  if(!m_device)
    return;

  for(uint32_t i = 0; i < m_cycleSize; i++)
  {
    vkDestroyFence(m_device, m_fences[i].fence, nullptr);
  }
  m_fences.clear();
  m_device = VK_NULL_HANDLE;
}

VkFence RingFences::getFence()
{
  m_fences[m_cycleIndex].active = true;
  return m_fences[m_cycleIndex].fence;
}


void RingFences::setCycleAndWait(uint32_t cycle)
{
  // set cycle
  m_cycleIndex = cycle % m_cycleSize;

  Entry& entry = m_fences[m_cycleIndex];
  if(entry.active)
  {
    // ensure the cycle we will use now has completed
    VkResult result = vkWaitForFences(m_device, 1, &entry.fence, VK_TRUE, ~0ULL);
    if(nvvk::checkResult(result, __FILE__, __LINE__))
    {
      exit(-1);
    }
    entry.active = false;
  }
  vkResetFences(m_device, 1, &entry.fence);
}

//////////////////////////////////////////////////////////////////////////

void RingCommandPool::init(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, uint32_t ringSize)
{
  assert(!m_device);
  m_device      = device;
  m_cycleIndex  = 0;
  m_cycleSize   = ringSize;
  m_flags       = flags;
  m_familyIndex = queueFamilyIndex;

  m_pools.resize(ringSize);
  for(uint32_t i = 0; i < m_cycleSize; i++)
  {
    VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.queueFamilyIndex        = queueFamilyIndex;
    info.flags                   = flags;

    NVVK_CHECK(vkCreateCommandPool(m_device, &info, nullptr, &m_pools[i].pool));
  }
}

void RingCommandPool::deinit()
{
  if(!m_device)
    return;

  for(uint32_t i = 0; i < m_cycleSize; i++)
  {
    Entry& entry = m_pools[i];
    if(!entry.cmds.empty())
    {
      vkFreeCommandBuffers(m_device, entry.pool, uint32_t(entry.cmds.size()), entry.cmds.data());
      vkResetCommandPool(m_device, entry.pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
      entry.cmds.clear();
    }
    vkDestroyCommandPool(m_device, entry.pool, nullptr);
  }

  m_device = VK_NULL_HANDLE;
}

void RingCommandPool::setCycle(uint32_t cycle)
{
  m_cycleIndex = cycle % m_cycleSize;

  Entry& entry = m_pools[m_cycleIndex];
  if(!entry.cmds.empty())
  {
    vkFreeCommandBuffers(m_device, entry.pool, uint32_t(entry.cmds.size()), entry.cmds.data());
    vkResetCommandPool(m_device, entry.pool, 0);
    entry.cmds.clear();
  }
}

VkCommandBuffer RingCommandPool::createCommandBuffer(VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
                                                     bool                 begin,
                                                     VkCommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/,
                                                     const VkCommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/)
{
  Entry& cycle = m_pools[m_cycleIndex];

  VkCommandBufferAllocateInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandBufferCount          = 1;
  info.commandPool                 = cycle.pool;
  info.level                       = level;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(m_device, &info, &cmd);

  cycle.cmds.push_back(cmd);

  if(begin)
  {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = flags;
    beginInfo.pInheritanceInfo         = pInheritanceInfo;

    vkBeginCommandBuffer(cmd, &beginInfo);
  }

  return cmd;
}

const VkCommandBuffer* RingCommandPool::createCommandBuffers(VkCommandBufferLevel level, uint32_t count)
{
  Entry& cycle = m_pools[m_cycleIndex];

  VkCommandBufferAllocateInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandBufferCount          = count;
  info.commandPool                 = cycle.pool;
  info.level                       = level;

  size_t begin = cycle.cmds.size();
  cycle.cmds.resize(begin + count);
  VkCommandBuffer* cmds = cycle.cmds.data() + begin;
  vkAllocateCommandBuffers(m_device, &info, cmds);

  return cmds;
}

//////////////////////////////////////////////////////////////////////////

void BatchSubmission::init(VkQueue queue)
{
  assert(m_waits.empty() && m_waitFlags.empty() && m_signals.empty() && m_commands.empty());
  m_queue = queue;
}

void BatchSubmission::enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers)
{
  for(uint32_t i = 0; i < num; i++)
  {
    m_commands.push_back(cmdbuffers[i]);
  }
}

void BatchSubmission::enqueue(VkCommandBuffer cmdbuffer)
{
  m_commands.push_back(cmdbuffer);
}

void BatchSubmission::enqueueSignal(VkSemaphore sem)
{
  m_signals.push_back(sem);
}

void BatchSubmission::enqueueWait(VkSemaphore sem, VkPipelineStageFlags flag)
{
  m_waits.push_back(sem);
  m_waitFlags.push_back(flag);
}

VkResult BatchSubmission::execute(VkFence fence /*= nullptr*/, uint32_t deviceMask)
{
  VkResult res = VK_SUCCESS;

  if(m_queue && (fence || !m_commands.empty() || !m_signals.empty() || !m_waits.empty()))
  {
    VkSubmitInfo submitInfo         = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount   = uint32_t(m_commands.size());
    submitInfo.signalSemaphoreCount = uint32_t(m_signals.size());
    submitInfo.waitSemaphoreCount   = uint32_t(m_waits.size());

    submitInfo.pCommandBuffers   = m_commands.data();
    submitInfo.pSignalSemaphores = m_signals.data();
    submitInfo.pWaitSemaphores   = m_waits.data();
    submitInfo.pWaitDstStageMask = m_waitFlags.data();

    std::vector<uint32_t> deviceMasks;
    std::vector<uint32_t> deviceIndices;

    VkDeviceGroupSubmitInfo deviceGroupInfo = {VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO};

    if(deviceMask != 0)
    {
      // Allocate an array big enough to hold the mask for all three parameters
      deviceMasks.resize(m_commands.size(), deviceMask);
      deviceIndices.resize(std::max(m_signals.size(), m_waits.size()), 0);  // Only perform semaphore actions on device zero

      submitInfo.pNext                              = &deviceGroupInfo;
      deviceGroupInfo.commandBufferCount            = submitInfo.commandBufferCount;
      deviceGroupInfo.pCommandBufferDeviceMasks     = deviceMasks.data();
      deviceGroupInfo.signalSemaphoreCount          = submitInfo.signalSemaphoreCount;
      deviceGroupInfo.pSignalSemaphoreDeviceIndices = deviceIndices.data();
      deviceGroupInfo.waitSemaphoreCount            = submitInfo.waitSemaphoreCount;
      deviceGroupInfo.pWaitSemaphoreDeviceIndices   = deviceIndices.data();
    }

    res = vkQueueSubmit(m_queue, 1, &submitInfo, fence);

    m_commands.clear();
    m_waits.clear();
    m_waitFlags.clear();
    m_signals.clear();
  }

  return res;
}


void BatchSubmission::waitIdle() const
{
  VkResult result = vkQueueWaitIdle(m_queue);
  if(nvvk::checkResult(result, __FILE__, __LINE__))
  {
    exit(-1);
  }
}

}  // namespace nvvk
