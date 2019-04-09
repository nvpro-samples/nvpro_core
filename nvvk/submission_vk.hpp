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

#pragma  once

#include <assert.h>
#include <platform.h>
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>


namespace nvvk {

  class BatchSubmission
  {
    /*
      Batches the submission arguments of VkSubmitInfo for VkQueueSubmit.

      vkQueueSubmit is a rather costly operation (depending on OS)
      and should mostly be avoided to be done too often. Therefore this utility
      class allows adding commandbuffers, semaphores etc. and submit in a batch.
      When using manual locks, it can also be useful to feed commandbuffers
      from different threads and then later kick it off.
    
    */

  private:
    VkQueue                           m_queue = nullptr;
    std::vector<VkSemaphore>          m_waits;
    std::vector<VkPipelineStageFlags> m_waitFlags;
    std::vector<VkSemaphore>          m_signals;
    std::vector<VkCommandBuffer>      m_commands;

  public:
    uint32_t getCommandBufferCount() const;

    // can change queue if nothing is pending
    void init(VkQueue queue);

    void enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers);
    void enqueue(VkCommandBuffer cmdbuffer);
    void enqueueAt(uint32_t pos, VkCommandBuffer cmdbuffer);
    void enqueueSignal(VkSemaphore sem);
    void enqueueWait(VkSemaphore sem, VkPipelineStageFlags flag);

    // submits the work and resets internal state
    VkResult execute(VkFence fence = nullptr, uint32_t deviceMask = 0);
  };


}

