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

#include "ringresources_vk.hpp"

namespace nvvk
{


  void RingFences::init(VkDevice device, const VkAllocationCallbacks* allocator /*= nullptr*/)
  {
    m_allocator = allocator;
    m_device = device;
    m_frame = 0;
    m_waited = 0;
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++)
    {
      VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
      info.flags = 0;
      VkResult result = vkCreateFence(device, &info, allocator, &m_fences[i]);
    }
  }

  void RingFences::deinit()
  {
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++)
    {
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
    if (m_waited == m_frame || m_frame < MAX_RING_FRAMES)
    {
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

    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++)
    {
      VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
      info.queueFamilyIndex = queueFamilyIndex;
      info.flags = flags;

      VkResult result = vkCreateCommandPool(m_device, &info, m_allocator, &m_cycles[i].pool);
    }
  }

  void RingCmdPool::deinit()
  {
    reset(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++)
    {
      vkDestroyCommandPool(m_device, m_cycles[i].pool, m_allocator);
    }
  }

  void RingCmdPool::reset(VkCommandPoolResetFlags flags)
  {
    for (uint32_t i = 0; i < MAX_RING_FRAMES; i++)
    {
      Cycle& cycle = m_cycles[i];
      if (m_dirty & (1 << i))
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
    if (m_dirty & (1 << cycleIndex))
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


}
