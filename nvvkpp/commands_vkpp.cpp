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


#include "commands_vkpp.hpp"
#include <nvvk/error_vk.hpp>


namespace nvvkpp {
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
#if VK_NV_device_generated_commands
      VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV,
      VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
      VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV,
      VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
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

void cmdBegin(vk::CommandBuffer cmd, vk::CommandBufferUsageFlags flags)
{
  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.flags = flags;

  cmd.begin(beginInfo);
}


//////////////////////////////////////////////////////////////////////////

void CmdPool::init(vk::Device device, uint32_t familyIndex, vk::CommandPoolCreateFlags flags)
{
  m_device                     = device;
  vk::CommandPoolCreateInfo info = {};
  info.flags                   = flags;
  info.queueFamilyIndex        = familyIndex;
  m_commandPool = m_device.createCommandPool(info, nullptr);
}

void CmdPool::deinit()
{
  if(m_commandPool)
    m_device.destroyCommandPool(m_commandPool, nullptr);
  m_commandPool = nullptr;
}

vk::CommandBuffer CmdPool::createCommandBuffer(vk::CommandBufferLevel level)
{
  vk::CommandBufferAllocateInfo allocInfo = {};
  allocInfo.level                       = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool                 = m_commandPool;
  allocInfo.commandBufferCount          = 1;

  vk::CommandBuffer commandBuffer;
  commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

  return commandBuffer;
}

vk::CommandBuffer CmdPool::createAndBegin(vk::CommandBufferLevel            level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
  vk::CommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/,
  vk::CommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/)
{
  vk::CommandBuffer cmd = createCommandBuffer(level);

  vk::CommandBufferBeginInfo beginInfo = {};
  beginInfo.flags                    = flags;
  beginInfo.pInheritanceInfo         = pInheritanceInfo;

  cmd.begin(beginInfo);

  return cmd;
}

void CmdPool::destroy(vk::CommandBuffer cmd)
{
  m_device.freeCommandBuffers(m_commandPool, 1, &cmd);
}

//////////////////////////////////////////////////////////////////////////

void ScopeSubmitCmdPool::end(vk::CommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  vk::SubmitInfo submitInfo       = {};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  m_queue.submit(submitInfo, nullptr);
  m_queue.waitIdle();

  CmdPool::destroy(commandBuffer);
}

//////////////////////////////////////////////////////////////////////////


void RingFences::init(vk::Device device)
{
  m_device = device;
  m_frame  = 0;
  m_waited = 0;
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    vk::FenceCreateInfo info = {};
    info.flags             = vk::FenceCreateFlags();
    m_fences[i] = device.createFence(info, nullptr);
  }
}

void RingFences::deinit()
{
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    m_device.destroyFence(m_fences[i], nullptr);
  }
}

void RingFences::reset()
{
  m_device.resetFences(MAX_RING_FRAMES, m_fences);
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

  while (m_device.waitForFences(m_fences[waitIndex], VK_TRUE, timeout) == vk::Result::eTimeout)
  {
    assert(!"getCmdBuffer: waitForFences failed - getCmdBuffer without submit? Not enough Command Buffers? Timeout too short?");
  }
  m_waited = m_frame;
}

vk::Fence RingFences::advanceCycle()
{
  vk::Fence fence = m_fences[m_frame % MAX_RING_FRAMES];
  m_device.resetFences(fence);
  m_frame++;

  return fence;
}

//////////////////////////////////////////////////////////////////////////

void RingCmdPool::init(vk::Device device, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags)
{
  m_device = device;
  m_dirty  = 0;
  m_index  = 0;

  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    vk::CommandPoolCreateInfo info = {};
    info.queueFamilyIndex        = queueFamilyIndex;
    info.flags                   = flags;

    m_cycles[i].pool = m_device.createCommandPool(info, nullptr);
  }
}

void RingCmdPool::deinit()
{
  reset(vk::CommandPoolResetFlagBits::eReleaseResources);
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    vkDestroyCommandPool(m_device, m_cycles[i].pool, nullptr);
  }
}

void RingCmdPool::reset(vk::CommandPoolResetFlags flags)
{
  for(uint32_t i = 0; i < MAX_RING_FRAMES; i++)
  {
    Cycle& cycle = m_cycles[i];
    if(m_dirty & (1 << i))
    {
      m_device.freeCommandBuffers(cycle.pool, uint32_t(cycle.cmds.size()), cycle.cmds.data());
      m_device.resetCommandPool(cycle.pool, flags);
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
    m_device.freeCommandBuffers(cycle.pool, uint32_t(cycle.cmds.size()), cycle.cmds.data());
    m_device.resetCommandPool(cycle.pool, vk::CommandPoolResetFlags());
    cycle.cmds.clear();
    m_dirty &= ~(1 << cycleIndex);
  }
  m_index = cycleIndex;
}

vk::CommandBuffer RingCmdPool::createCommandBuffer(vk::CommandBufferLevel level)
{
  Cycle& cycle = m_cycles[m_index];

  vk::CommandBufferAllocateInfo info = {};
  info.commandBufferCount          = 1;
  info.commandPool                 = cycle.pool;
  info.level                       = level;

  vk::CommandBuffer cmd;
  cmd = m_device.allocateCommandBuffers(info)[0];

  cycle.cmds.push_back(cmd);

  m_dirty |= (1 << m_index);
  return cmd;
}

vk::CommandBuffer RingCmdPool::createAndBegin(vk::CommandBufferLevel            level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
  vk::CommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/,
  vk::CommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/)
{
  vk::CommandBuffer cmd = createCommandBuffer(level);

  vk::CommandBufferBeginInfo beginInfo = {};
  beginInfo.flags                    = flags;
  beginInfo.pInheritanceInfo         = pInheritanceInfo;

  cmd.begin(beginInfo);

  return cmd;
}

const vk::CommandBuffer* RingCmdPool::createCommandBuffers(vk::CommandBufferLevel level, uint32_t count)
{
  Cycle& cycle = m_cycles[m_index];

  vk::CommandBufferAllocateInfo info = {};
  info.commandBufferCount          = count;
  info.commandPool                 = cycle.pool;
  info.level                       = level;

  size_t begin = cycle.cmds.size();
  cycle.cmds.resize(begin + count);
  vk::CommandBuffer* cmds = cycle.cmds.data() + begin;
  m_device.allocateCommandBuffers(&info, cmds);

  return cmds;
}

//////////////////////////////////////////////////////////////////////////

void BatchSubmission::init(vk::Queue queue)
{
  assert(m_waits.empty() && m_waitFlags.empty() && m_signals.empty() && m_commands.empty());
  m_queue = queue;
}

void BatchSubmission::enqueue(uint32_t num, const vk::CommandBuffer* cmdbuffers)
{
  m_commands.reserve(m_commands.size() + num);
  for(uint32_t i = 0; i < num; i++)
  {
    m_commands.push_back(cmdbuffers[i]);
  }
}

void BatchSubmission::enqueue(vk::CommandBuffer cmdbuffer)
{
  m_commands.push_back(cmdbuffer);
}

void BatchSubmission::enqueueSignal(vk::Semaphore sem)
{
  m_signals.push_back(sem);
}

void BatchSubmission::enqueueWait(vk::Semaphore sem, vk::PipelineStageFlags flag)
{
  m_waits.push_back(sem);
  m_waitFlags.push_back(flag);
}

vk::Result BatchSubmission::execute(vk::Fence fence /*= nullptr*/, uint32_t deviceMask)
{
  vk::Result res = vk::Result::eSuccess;

  if(m_queue && (fence || !m_commands.empty() || !m_signals.empty() || !m_waits.empty()))
  {
    vk::SubmitInfo submitInfo         = {};
    submitInfo.commandBufferCount   = uint32_t(m_commands.size());
    submitInfo.signalSemaphoreCount = uint32_t(m_signals.size());
    submitInfo.waitSemaphoreCount   = uint32_t(m_waits.size());

    submitInfo.pCommandBuffers   = m_commands.data();
    submitInfo.pSignalSemaphores = m_signals.data();
    submitInfo.pWaitSemaphores   = m_waits.data();
    submitInfo.pWaitDstStageMask = m_waitFlags.data();

    std::vector<uint32_t> deviceMasks;
    std::vector<uint32_t> deviceIndices;

    vk::DeviceGroupSubmitInfo deviceGroupInfo = {};

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

    m_queue.submit(submitInfo, fence);

    m_commands.clear();
    m_waits.clear();
    m_waitFlags.clear();
    m_signals.clear();
  }

  return res;
}


}  // namespace nvvk
