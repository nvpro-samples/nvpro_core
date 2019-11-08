/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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

#include <algorithm>
#include <platform.h>


#include "commands_vk.hpp"
#include "error_vk.hpp"


namespace nvvk {
uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask)
{
  static const uint32_t accessPipes[] = {
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
      VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
      VK_ACCESS_INDEX_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_UNIFORM_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
          | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
          | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
          | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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

void CmdPool::init(VkDevice device, uint32_t familyIndex, VkCommandPoolCreateFlags flags)
{
  m_device                     = device;
  VkCommandPoolCreateInfo info = {};
  info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.flags                   = flags;
  info.queueFamilyIndex        = familyIndex;
  vkCreateCommandPool(m_device, &info, nullptr, &m_commandPool);
}

void CmdPool::deinit()
{
  if(m_commandPool)
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  m_commandPool = VK_NULL_HANDLE;
}

VkCommandBuffer CmdPool::createCommandBuffer(VkCommandBufferLevel level)
{
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool                 = m_commandPool;
  allocInfo.commandBufferCount          = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

  return commandBuffer;
}

VkCommandBuffer CmdPool::createAndBegin(VkCommandBufferLevel            level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
                                        VkCommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/,
                                        VkCommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/)
{
  VkCommandBuffer cmd = createCommandBuffer(level);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = flags;
  beginInfo.pInheritanceInfo         = pInheritanceInfo;

  vkBeginCommandBuffer(cmd, &beginInfo);

  return cmd;
}

void CmdPool::destroy(VkCommandBuffer cmd)
{
  vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

//////////////////////////////////////////////////////////////////////////

void ScopeSubmitCmdPool::end(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
  VkResult result = vkQueueWaitIdle(m_queue);
  if(nvvk::checkResult(result, __FILE__, __LINE__))
  {
    exit(-1);
  }

  CmdPool::destroy(commandBuffer);
}

//////////////////////////////////////////////////////////////////////////


void RingFences::init(VkDevice device)
{
  m_device = device;
  m_frame  = 0;
  m_waited = 0;
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    VkFenceCreateInfo info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    info.flags             = 0;
    VkResult result        = vkCreateFence(device, &info, nullptr, &m_fences[i]);
  }
}

void RingFences::deinit()
{
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    vkDestroyFence(m_device, m_fences[i], nullptr);
  }
}

void RingFences::reset()
{
  vkResetFences(m_device, MAX_RING_FRAMES, m_fences);
  m_frame  = 0;
  m_waited = m_frame;
}

void RingFences::wait(uint64_t timeout /*= ~0ULL*/)
{
  if(m_waited == m_frame || m_frame < MAX_RING_FRAMES)
  {
    return;
  }

  uint32_t waitIndex = (m_frame) % MAX_RING_FRAMES;

  VkResult result = vkWaitForFences(m_device, 1, &m_fences[waitIndex], VK_TRUE, timeout);
  if(nvvk::checkResult(result, __FILE__, __LINE__))
  {
    exit(-1);
  }
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

void RingCmdPool::init(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
  m_device = device;
  m_dirty  = 0;
  m_index  = 0;

  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.queueFamilyIndex        = queueFamilyIndex;
    info.flags                   = flags;

    VkResult result = vkCreateCommandPool(m_device, &info, nullptr, &m_cycles[i].pool);
  }
}

void RingCmdPool::deinit()
{
  reset(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    vkDestroyCommandPool(m_device, m_cycles[i].pool, nullptr);
  }
}

void RingCmdPool::reset(VkCommandPoolResetFlags flags)
{
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    Cycle& cycle = m_cycles[i];
    if(m_dirty & (1 << i))
    {
      vkFreeCommandBuffers(m_device, cycle.pool, uint32_t(cycle.cmds.size()), cycle.cmds.data());
      vkResetCommandPool(m_device, cycle.pool, flags);
      cycle.cmds.clear();
    }
  }

  m_dirty = 0;
}

void RingCmdPool::setCycle(uint32_t cycleIndex)
{
  if(m_dirty & (1 << cycleIndex))
  {
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

  VkCommandBufferAllocateInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandBufferCount          = 1;
  info.commandPool                 = cycle.pool;
  info.level                       = level;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(m_device, &info, &cmd);

  cycle.cmds.push_back(cmd);

  m_dirty |= (1 << m_index);
  return cmd;
}

VkCommandBuffer RingCmdPool::createAndBegin(VkCommandBufferLevel            level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
                                            VkCommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/,
                                            VkCommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/)
{
  VkCommandBuffer cmd = createCommandBuffer(level);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = flags;
  beginInfo.pInheritanceInfo         = pInheritanceInfo;

  vkBeginCommandBuffer(cmd, &beginInfo);

  return cmd;
}

const VkCommandBuffer* RingCmdPool::createCommandBuffers(VkCommandBufferLevel level, uint32_t count)
{
  Cycle& cycle = m_cycles[m_index];

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
  m_commands.reserve(m_commands.size() + num);
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


}  // namespace nvvk
