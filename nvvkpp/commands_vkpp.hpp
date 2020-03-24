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
#include <iostream>
#include <vulkan/vulkan.hpp>

namespace nvvkpp {

//--------------------------------------------------------------------------------------------------
/**
# class nvvkpp::SingleCommandBuffer

With `SingleCommandBuffer`, you create the the command buffer by calling `createCommandBuffer()` and submit all the work by calling `flushCommandBuffer()`

~~~~ c++
nvvkpp::SingleCommandBuffer sc(m_device, m_graphicsQueueIndex);
vk::CommandBuffer cmdBuf = sc.createCommandBuffer();
...
sc.flushCommandBuffer(cmdBuf);
~~~~
*/
class SingleCommandBuffer
{
public:
  SingleCommandBuffer(const vk::Device& device, uint32_t familyQueueIndex)
      : m_device(device)
  {
    m_queue   = m_device.getQueue(familyQueueIndex, 0);
    m_cmdPool = m_device.createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, familyQueueIndex});
  }

  ~SingleCommandBuffer() { m_device.destroyCommandPool(m_cmdPool); }

  vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const
  {
    vk::CommandBuffer cmdBuffer = m_device.allocateCommandBuffers({m_cmdPool, level, 1})[0];
    cmdBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    return cmdBuffer;
  }

  void flushCommandBuffer(const vk::CommandBuffer& commandBuffer) const
  {
    commandBuffer.end();
    m_queue.submit(vk::SubmitInfo{0, nullptr, nullptr, 1, &commandBuffer}, vk::Fence());
    m_queue.waitIdle();
    m_device.freeCommandBuffers(m_cmdPool, commandBuffer);
  }


private:
  const vk::Device& m_device;
  vk::CommandPool   m_cmdPool;
  vk::Queue         m_queue;
};

//--------------------------------------------------------------------------------------------------
/**
# class nvvkpp::ScopeCommandBuffer
The `ScopeCommandBuffer` is similar, but the creation and flush is automatic. The submit of the operations will be done when it goes out of scope.

~~~~ c++
{
  nvvkpp::ScopeCommandBuffer cmdBuf(m_device, m_graphicQueueIndex);
  functionWithCommandBufferInParameter(cmdBuf);
} // internal functions are executed
~~~~

> Note: The above methods are not good for performance critical areas as it is stalling the execution.
*/
class ScopeCommandBuffer : public SingleCommandBuffer
{
public:
  ScopeCommandBuffer(const vk::Device& device, uint32_t familyQueueIndex)
      : SingleCommandBuffer(device, familyQueueIndex)
  {
    m_cmdBuf = createCommandBuffer();
  }
  ~ScopeCommandBuffer() { flushCommandBuffer(m_cmdBuf); }

  operator vk::CommandBuffer() { return m_cmdBuf; }

private:
  vk::CommandBuffer m_cmdBuf;
};


//--------------------------------------------------------------------------------------------------
/**
# class nvvkpp::MultipleCommandBuffers
This is the suggested way to use command buffers while building up the scene. The reason is it will 
not be blocking and will transfer the staging buffers in a different thread. There are by default 10
command buffers which could be in theory executed in parallel.

**Setup:** create one instance of it, as a member of the class and pass it around. You will need the
vk::Device and the family queue index.

**Get:** call `getCmdBuffer()` for the next available command buffer.

**Submit:** `submit()` will submit the current active command buffer. It will return a fence which 
can be used for flushing the staging buffers. See previous chapter.

**Flushing the Queue**: in case there are still pending commands, you can call `waitForUpload()` 
and this will make sure that the queue is idle.

~~~~C++
  nvvkpp::MultipleCommandBuffers m_cmdBufs;
  m_cmdBufs.setup(device, graphicsQueueIndex);
  ...
  auto cmdBuf = m_cmdBufs.getCmdBuffer();
  // Create buffers
  // Create images
  auto fence = m_cmdBufs.submit();
  m_alloc.flushStaging(fence);
~~~~
*/
class MultipleCommandBuffers
{
public:
  void setup(const vk::Device& device, uint32_t familyQueueIndex, uint32_t nbCmdBuf = 10)
  {
    m_nbCmdBuf = nbCmdBuf;
    m_cmdBuffers.resize(m_nbCmdBuf);
    m_fences.resize(m_nbCmdBuf);

    m_device     = device;
    m_queue      = m_device.getQueue(familyQueueIndex, 0);
    m_cmdPool    = m_device.createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, familyQueueIndex});
    m_cmdBuffers = m_device.allocateCommandBuffers({m_cmdPool, vk::CommandBufferLevel::ePrimary, m_nbCmdBuf});
    for(uint32_t i = 0; i < m_nbCmdBuf; i++)
    {
      m_fences[i] = device.createFence({vk::FenceCreateFlagBits::eSignaled});
    }
  }

  void destroy()
  {
    for(auto& f : m_fences)
    {
      m_device.destroyFence(f);
    }
    m_device.freeCommandBuffers(m_cmdPool, m_cmdBuffers);
    m_device.destroyCommandPool(m_cmdPool);
  }

  vk::CommandBuffer getCmdBuffer(vk::CommandBufferUsageFlagBits f = vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
  {
    while(m_device.waitForFences(m_fences[m_curCmd], VK_TRUE, 10) == vk::Result::eTimeout)
    {
      assert(!"getCmdBuffer: waitForFences failed - getCmdBuffer without submit? Not enough Command Buffers? Timeout too short?");
    }
    m_device.resetFences(m_fences[m_curCmd]);
    m_cmdBuffers[m_curCmd].begin(vk::CommandBufferBeginInfo{f});
    return m_cmdBuffers[m_curCmd];
  }

  vk::Fence submit()
  {
    vk::Fence curFence{m_fences[m_curCmd]};
    m_cmdBuffers[m_curCmd].end();
    m_queue.submit(vk::SubmitInfo{0, nullptr, nullptr, 1, &m_cmdBuffers[m_curCmd]}, curFence);

    m_curCmd = (m_curCmd + 1) % m_nbCmdBuf;
    return curFence;
  }

  // Making sure everything is uploaded. Don't use this often.
  void waitForUpload() { m_queue.waitIdle(); }


private:
  vk::Device                     m_device;
  vk::CommandPool                m_cmdPool;
  vk::Queue                      m_queue;
  std::vector<vk::CommandBuffer> m_cmdBuffers;
  std::vector<vk::Fence>         m_fences;
  uint32_t                       m_curCmd{0};
  uint32_t                       m_nbCmdBuf{0};
};

/**
  # functions in nvvk

  - makeAccessMaskPipelineStageFlags : depending on accessMask returns appropriate vk::PipelineStageFlagBits
  - cmdBegin : wraps vkBeginCommandBuffer with vk::CommandBufferUsageFlags and implicitly handles vk::CommandBufferBeginInfo setup
  - makeSubmitInfo : vk::SubmitInfo struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed
*/

// useful for barriers, derive all compatible stage flags from an access mask

uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask);

inline vk::SubmitInfo makeSubmitInfo(uint32_t numCmds, vk::CommandBuffer* cmds, uint32_t numSignals, vk::Semaphore* signals) {
  vk::SubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
  submitInfo.pCommandBuffers = cmds;
  submitInfo.commandBufferCount = numCmds;
  submitInfo.pSignalSemaphores = signals;
  submitInfo.signalSemaphoreCount = numSignals;

  return submitInfo;
}

//////////////////////////////////////////////////////////////////////////

/**
  # class nvvkpp::CmdPool

  CmdPool stores a single vk::CommandPool and provides utility functions
  to create vk::CommandBuffers from it.
*/

class CmdPool
{
public:
  void init(vk::Device                     device,
    uint32_t                     familyIndex,
    vk::CommandPoolCreateFlags     flags = vk::CommandPoolCreateFlagBits::eTransient);
  void deinit();

  vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

  // allocates and begins cmdbuffer
  vk::CommandBuffer createAndBegin(vk::CommandBufferLevel            level = vk::CommandBufferLevel::ePrimary,
    vk::CommandBufferInheritanceInfo* pInheritanceInfo = nullptr,
    vk::CommandBufferUsageFlags       flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  // free cmdbuffer from this pool
  void destroy(vk::CommandBuffer cmd);

private:
  vk::Device                     m_device;
  uint32_t                     m_familyIndex = ~0;
  vk::CommandPool                m_commandPool;
};


//////////////////////////////////////////////////////////////////////////

/**
  # class nvvkpp::ScopeSubmitCmdPool

  ScopeSubmitCmdPool extends CmdPool and lives within a scope.
  It directly submits its commandbufers to the provided queue.
  Intent is for non-critical actions where performance is NOT required,
  as it waits until the device has completed the operation.

  Example:
  ``` C++
  {
    nvvkpp::ScopeSubmitCmdPool scopePool(...);

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
  ScopeSubmitCmdPool(vk::Device device, vk::Queue queue, uint32_t familyIndex)
  {
    init(device, familyIndex, vk::CommandPoolCreateFlagBits::eTransient);
    m_queue = queue;
  }

  ~ScopeSubmitCmdPool() { deinit(); }

  vk::CommandBuffer begin()
  {
    return CmdPool::createAndBegin(vk::CommandBufferLevel::ePrimary, nullptr, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  }

  // ends cmdbuffer, submits it on queue and waits for queue to be finished, frees cmdbuffer
  void end(vk::CommandBuffer commandBuffer);

private:
  vk::Device                     m_device;
  vk::Queue                      m_queue;
  uint32_t                     m_familyIndex;
  vk::CommandPool                m_commandPool;
};

/**
  # class nvvkpp::ScopeSubmitCmdBuffer

  Provides a single vk::CommandBuffer that lives within the scope
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
  ScopeSubmitCmdBuffer(vk::Device device, vk::Queue queue, uint32_t familyIndex)
    : ScopeSubmitCmdPool(device, queue, familyIndex)
  {
    m_cmd = begin();
  }

  ~ScopeSubmitCmdBuffer() { end(m_cmd); }

  operator vk::CommandBuffer() const { return m_cmd; };

private:
  vk::CommandBuffer m_cmd;
};

//////////////////////////////////////////////////////////////////////////
/**
  # classes **nvvkpp::Ring...**

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
  ## class nvvkpp::RingFences

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
  void init(vk::Device device);
  void deinit();
  void reset();

  // waits until current cycle can be safely used
  // can call multiple times, will skip wait if already used in same frame
  void wait(uint64_t timeout = ~0ULL);

  // query current cycle index
  uint32_t getCycleIndex() const { return m_frame % MAX_RING_FRAMES; }

  // call once per cycle at end of frame
  vk::Fence advanceCycle();

private:
  uint32_t                     m_frame;
  uint32_t                     m_waited;
  vk::Fence                      m_fences[MAX_RING_FRAMES];
  vk::Device                     m_device;
};

/**
  ## class nvvkpp::RingCmdPool

  Manages a fixed cycle set of vk::CommandBufferPools and
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

    vk::CommandBuffer cmd = ringPool.createCommandBuffer(...)
    ... do stuff / submit etc...

    vk::Fence fence = ringFences.advanceCycle();
    // use this fence in the submit
    vkQueueSubmit(...)
  }
  ~~~
*/

class RingCmdPool
{
public:
  void init(vk::Device                     device,
    uint32_t                     queueFamilyIndex,
    vk::CommandPoolCreateFlags     flags = vk::CommandPoolCreateFlagBits::eTransient);
  void deinit();
  void reset(vk::CommandPoolResetFlags flags = vk::CommandPoolResetFlags() );

  // call once per cycle prior creating command buffers
  // resets old pools etc.
  void setCycle(uint32_t cycleIndex);

  // ensure proper cycle is set prior this
  vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level);

  vk::CommandBuffer createAndBegin(vk::CommandBufferLevel            level = vk::CommandBufferLevel::ePrimary,
    vk::CommandBufferInheritanceInfo* pInheritanceInfo = nullptr,
    vk::CommandBufferUsageFlags       flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  // pointer is only valid until next create
  const vk::CommandBuffer* createCommandBuffers(vk::CommandBufferLevel level, uint32_t count);

private:
  struct Cycle
  {
    vk::CommandPool                pool;
    std::vector<vk::CommandBuffer> cmds;
  };

  Cycle                        m_cycles[MAX_RING_FRAMES];
  vk::Device                     m_device;
  uint32_t                     m_index;
  uint32_t                     m_dirty;
};


//////////////////////////////////////////////////////////////////////////
/**
  # class nvvkpp::BatchSubmission

  Batches the submission arguments of vk::SubmitInfo for vk::QueueSubmit.

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
  vk::Queue                           m_queue = nullptr;
  std::vector<vk::Semaphore>          m_waits;
  std::vector<vk::PipelineStageFlags> m_waitFlags;
  std::vector<vk::Semaphore>          m_signals;
  std::vector<vk::CommandBuffer>      m_commands;

public:
  uint32_t getCommandBufferCount() const { return uint32_t(m_commands.size()); }
  vk::Queue  getQueue() const { return m_queue; }

  // can change queue if nothing is pending
  void init(vk::Queue queue);

  void enqueue(uint32_t num, const vk::CommandBuffer* cmdbuffers);
  void enqueue(vk::CommandBuffer cmdbuffer);
  void enqueueSignal(vk::Semaphore sem);
  void enqueueWait(vk::Semaphore sem, vk::PipelineStageFlags flag);

  // submits the work and resets internal state
  vk::Result execute(vk::Fence fence = nullptr, uint32_t deviceMask = 0);

  void waitIdle() const { vkQueueWaitIdle(m_queue); }
};


}  // namespace nvvkpp
