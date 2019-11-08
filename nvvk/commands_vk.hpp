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

#pragma once

#include <platform.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {

//////////////////////////////////////////////////////////////////////////

/**
  # functions in nvvk

  - makeAccessMaskPipelineStageFlags : depending on accessMask returns appropriate VkPipelineStageFlagBits
  - cmdBegin : wraps vkBeginCommandBuffer with VkCommandBufferUsageFlags and implicitly handles VkCommandBufferBeginInfo setup
  - makeSubmitInfo : VkSubmitInfo struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed
*/

// useful for barriers, derive all compatible stage flags from an access mask

uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask);

void cmdBegin(VkCommandBuffer cmd, VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

NV_INLINE VkSubmitInfo makeSubmitInfo(uint32_t numCmds, VkCommandBuffer* cmds, uint32_t numSignals, VkSemaphore* signals) {
  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.pCommandBuffers = cmds;
  submitInfo.commandBufferCount = numCmds;
  submitInfo.pSignalSemaphores = signals;
  submitInfo.signalSemaphoreCount = numSignals;

  return submitInfo;
}

//////////////////////////////////////////////////////////////////////////

/**
  # class nvvk::CmdPool

  CmdPool stores a single VkCommandPool and provides utility functions
  to create VkCommandBuffers from it.
*/

class CmdPool
{
public:
  void init(VkDevice                     device,
            uint32_t                     familyIndex,
            VkCommandPoolCreateFlags     flags     = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
  void deinit();

  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  // allocates and begins cmdbuffer
  VkCommandBuffer createAndBegin(VkCommandBufferLevel            level            = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                 VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr,
                                 VkCommandBufferUsageFlags       flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  // free cmdbuffer from this pool
  void destroy(VkCommandBuffer cmd);

private:
  VkDevice                     m_device = VK_NULL_HANDLE;
  uint32_t                     m_familyIndex = ~0;
  VkCommandPool                m_commandPool = VK_NULL_HANDLE;
};


//////////////////////////////////////////////////////////////////////////

/**
  # class nvvk::ScopeSubmitCmdPool

  ScopeSubmitCmdPool extends CmdPool and lives within a scope.
  It directly submits its commandbufers to the provided queue.
  Intent is for non-critical actions where performance is NOT required,
  as it waits until the device has completed the operation.

  Example:
  ``` C++
  {
    nvvk::ScopeSubmitCmdPool scopePool(...);

    // some batch of work
    {
      vkCommandBuffer cmd = scopePool.begin();
      ... record commands ...
      // blocking operation
      scopePool.end(cmd);
    }

    // other operations done here
    {
      vkCommandBuffer cmd = scopePool.begin();
      ... record commands ...
      // blocking operation
      scopePool.end(cmd);
    }
  }
  ```
*/

class ScopeSubmitCmdPool : private CmdPool
{
public:
  ScopeSubmitCmdPool(VkDevice device, VkQueue queue, uint32_t familyIndex)
  {
    init(device, familyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    m_queue = queue;
  }

  ~ScopeSubmitCmdPool() { deinit(); }

  VkCommandBuffer begin()
  {
    return CmdPool::createAndBegin(VK_COMMAND_BUFFER_LEVEL_PRIMARY, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  }

  // ends cmdbuffer, submits it on queue and waits for queue to be finished, frees cmdbuffer
  void end(VkCommandBuffer commandBuffer);

private:
  VkDevice                     m_device;
  VkQueue                      m_queue;
  uint32_t                     m_familyIndex;
  VkCommandPool                m_commandPool;
};

/**
  # class nvvk::ScopeSubmitCmdBuffer

  Provides a single VkCommandBuffer that lives within the scope
  and is directly submitted, deleted when the scope is left.

  Example:
  ``` C++
  {
    ScopeSubmitCmdBuffer cmd(device, queue, queueFamilyIndex);
    ... do stuff
    vkCmdCopyBuffer(cmd, ...);
  }
  ```
*/

class ScopeSubmitCmdBuffer : public ScopeSubmitCmdPool
{
public:
  ScopeSubmitCmdBuffer(VkDevice device, VkQueue queue, uint32_t familyIndex)
      : ScopeSubmitCmdPool(device, queue, familyIndex)
  {
    m_cmd = begin();
  }

  ~ScopeSubmitCmdBuffer() { end(m_cmd); }

  operator VkCommandBuffer() const { return m_cmd; };

private:
  VkCommandBuffer m_cmd;
};

//////////////////////////////////////////////////////////////////////////
/**
  # classes **nvvk::Ring...**

  In real-time processing, the CPU typically generates commands 
  in advance to the GPU and send them in batches for exeuction.

  To avoid having he CPU to wait for the GPU'S completion and let it "race ahead"
  we make use of double, or tripple-buffering techniques, where we cycle through
  a pool of resources every frame. We know that those resources are currently 
  not in use by the GPU and can therefore manipulate them directly.
  
  Especially in Vulkan it is the developer's responsibility to avoid such
  access of resources that are in-flight.

  The "Ring" classes cycle through a pool of 3 resources, as that is typically
  the maximum latency drivers may let the CPU get in advance of the GPU.
*/

static const uint32_t MAX_RING_FRAMES = 3;

/**
  ## class nvvk::RingFences

  Recycles a fixed number of fences, provides information in which cycle
  we are currently at, and prevents accidental access to a cycle inflight.

  A typical frame would start by waiting for older cycle's completion ("wait")
  and be ended by "advanceCycle".

  Safely index other resources, for example ring buffers using the "getCycleIndex"
  for the current frame.
*/

class RingFences
{
public:
  void init(VkDevice device);
  void deinit();
  void reset();

  // waits until current cycle can be safely used
  // can call multiple times, will skip wait if already used in same frame
  void wait(uint64_t timeout = ~0ULL);

  // query current cycle index
  uint32_t getCycleIndex() const { return m_frame % MAX_RING_FRAMES; }

  // call once per cycle at end of frame
  VkFence advanceCycle();

private:
  uint32_t                     m_frame;
  uint32_t                     m_waited;
  VkFence                      m_fences[MAX_RING_FRAMES];
  VkDevice                     m_device;
};

/**
  ## class nvvk::RingCmdPool

  Manages a fixed cycle set of VkCommandBufferPools and
  one-shot command buffers allocated from them.

  Every cycle a different command buffer pool is used for
  providing the command buffers. Command buffers are automatically
  deleted after a full cycle (MAX_RING_FRAMES) has been completed.

  The usage of multiple command buffer pools also means we get nice allocation
  behavior (linear allocation from frame start to frame end) without fragmentation.
  If we were using a single command pool, it would fragment easily.

  Example:

  ~~~ C++
  {
    // wait until we can use the new cycle (normally we never have to wait)
    ringFences.wait();

    ringPool.setCycle( ringFences.getCycleIndex() )

    VkCommandBuffer cmd = ringPool.createCommandBuffer(...)
    ... do stuff / submit etc...

    VkFence fence = ringFences.advanceCycle();
    // use this fence in the submit
    vkQueueSubmit(...)
  }
  ~~~
*/

class RingCmdPool
{
public:
  void init(VkDevice                     device,
            uint32_t                     queueFamilyIndex,
            VkCommandPoolCreateFlags     flags     = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
  void deinit();
  void reset(VkCommandPoolResetFlags flags = 0);

  // call once per cycle prior creating command buffers
  // resets old pools etc.
  void setCycle(uint32_t cycleIndex);

  // ensure proper cycle is set prior this
  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level);

  VkCommandBuffer createAndBegin(VkCommandBufferLevel            level            = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                 VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr,
                                 VkCommandBufferUsageFlags       flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  // pointer is only valid until next create
  const VkCommandBuffer* createCommandBuffers(VkCommandBufferLevel level, uint32_t count);

private:
  struct Cycle
  {
    VkCommandPool                pool;
    std::vector<VkCommandBuffer> cmds;
  };

  Cycle                        m_cycles[MAX_RING_FRAMES];
  VkDevice                     m_device;
  uint32_t                     m_index;
  uint32_t                     m_dirty;
};


//////////////////////////////////////////////////////////////////////////
/**
  # class nvvk::BatchSubmission

  Batches the submission arguments of VkSubmitInfo for VkQueueSubmit.

  vkQueueSubmit is a rather costly operation (depending on OS)
  and should be avoided to be done too often (< 10). Therefore this utility
  class allows adding commandbuffers, semaphores etc. and submit in a batch.

  When using manual locks, it can also be useful to feed commandbuffers
  from different threads and then later kick it off.

  Example

  ~~~ C++
    // within upload logic
    {
      semTransfer = handleUpload(...);
      // for example trigger async upload on transfer queue here
      vkQueueSubmit(..);

      // tell next frame's batch submission 
      // that its commandbuffers should wait for transfer
      // to be completed
      graphicsSubmission.enqueWait(semTransfer)
    }

    // within present logic
    {
      // for example ensure the next frame waits until proper present semaphore was triggered
      graphicsSubmission.enqueueWait(presentSemaphore);
    }

    // within drawing logic
    {
      // enqeue some graphics work for submission
      graphicsSubmission.enqueue(getSceneCmdBuffer());
      graphicsSubmission.enqueue(getUiCmdBuffer());

      graphicsSubmission.execute(frameFence);
    }
  ~~~
*/

class BatchSubmission
{
private:
  VkQueue                           m_queue = nullptr;
  std::vector<VkSemaphore>          m_waits;
  std::vector<VkPipelineStageFlags> m_waitFlags;
  std::vector<VkSemaphore>          m_signals;
  std::vector<VkCommandBuffer>      m_commands;

public:
  uint32_t getCommandBufferCount() const { return uint32_t(m_commands.size()); }
  VkQueue  getQueue() const { return m_queue; }

  // can change queue if nothing is pending
  void init(VkQueue queue);

  void enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers);
  void enqueue(VkCommandBuffer cmdbuffer);
  void enqueueSignal(VkSemaphore sem);
  void enqueueWait(VkSemaphore sem, VkPipelineStageFlags flag);

  // submits the work and resets internal state
  VkResult execute(VkFence fence = nullptr, uint32_t deviceMask = 0);

  void waitIdle() const { vkQueueWaitIdle(m_queue); }
};
}  // namespace nvvk
