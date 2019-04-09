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

#include <algorithm>
#include "submission_vk.hpp"

namespace nvvk {

  void BatchSubmission::init(VkQueue queue)
  {
    assert(m_waits.empty() && m_waitFlags.empty() && m_signals.empty() && m_commands.empty());
    m_queue = queue;
  }

  uint32_t BatchSubmission::getCommandBufferCount() const
  {
    return uint32_t(m_commands.size());
  }

  void BatchSubmission::enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers)
  {
    m_commands.reserve(m_commands.size() + num);
    for (uint32_t i = 0; i < num; i++)
    {
      m_commands.push_back(cmdbuffers[i]);
    }
  }

  void BatchSubmission::enqueue(VkCommandBuffer cmdbuffer)
  {
    m_commands.push_back(cmdbuffer);
  }

  void BatchSubmission::enqueueAt(uint32_t pos, VkCommandBuffer cmdbuffer)
  {
    m_commands.insert(m_commands.begin() + pos, cmdbuffer);
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

    if (m_queue && (fence || !m_commands.empty() || !m_signals.empty() || !m_waits.empty()))
    {
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
        deviceIndices.resize(std::max(m_signals.size(), m_waits.size()), 0);  // Only perform semaphore actions on device zero

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
}
