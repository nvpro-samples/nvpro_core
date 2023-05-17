/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <platform.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {

//--------------------------------------------------------------------------------------------------
/**
# functions in nvvk

- makeAccessMaskPipelineStageFlags : depending on accessMask returns appropriate VkPipelineStageFlagBits
- cmdBegin : wraps vkBeginCommandBuffer with VkCommandBufferUsageFlags and implicitly handles VkCommandBufferBeginInfo setup
- makeSubmitInfo : VkSubmitInfo struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed
*/

// useful for barriers, derive all compatible stage flags from an access mask


uint32_t makeAccessMaskPipelineStageFlags(uint32_t accessMask,
                                          VkPipelineStageFlags supportedShaderBits = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
                                                                                     | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
                                                                                     | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                                                                     | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

void cmdBegin(VkCommandBuffer cmd, VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

inline VkSubmitInfo makeSubmitInfo(uint32_t numCmds, VkCommandBuffer* cmds, uint32_t numSignals, VkSemaphore* signals)
{
  VkSubmitInfo submitInfo         = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.pCommandBuffers      = cmds;
  submitInfo.commandBufferCount   = numCmds;
  submitInfo.pSignalSemaphores    = signals;
  submitInfo.signalSemaphoreCount = numSignals;

  return submitInfo;
}

//--------------------------------------------------------------------------------------------------
/**
  \class nvvk::CommandPool

  nvvk::CommandPool stores a single VkCommandPool and provides utility functions
  to create VkCommandBuffers from it.

  Example:
  \code{.cpp}
  {
    nvvk::CommandPool cmdPool;
    cmdPool.init(...);

    // some setup/one shot work
    {
      vkCommandBuffer cmd = scopePool.createAndBegin();
      ... record commands ...
      // trigger execution with a blocking operation
      // not recommended for performance
      // but useful for sample setup
      scopePool.submitAndWait(cmd, queue);
    }

    // other cmds you may batch, or recycle
    std::vector<VkCommandBuffer> cmds;
    {
      vkCommandBuffer cmd = scopePool.createAndBegin();
      ... record commands ...
      cmds.push_back(cmd);
    }
    {
      vkCommandBuffer cmd = scopePool.createAndBegin();
      ... record commands ...
      cmds.push_back(cmd);
    }

    // do some form of batched submission of cmds

    // after completion destroy cmd
    cmdPool.destroy(cmds.size(), cmds.data());
    cmdPool.deinit();
  }
  \endcode
*/

class CommandPool
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
    init(device, familyIndex, flags, defaultQueue);
  }

  // if defaultQueue is null, uses first queue from familyIndex as default
  void init(VkDevice                 device,
            uint32_t                 familyIndex,
            VkCommandPoolCreateFlags flags        = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            VkQueue                  defaultQueue = VK_NULL_HANDLE);
  void deinit();


  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel      level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                      bool                      begin = true,
                                      VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                      const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr);

  // free cmdbuffers from this pool
  void destroy(size_t count, const VkCommandBuffer* cmds);
  void destroy(const std::vector<VkCommandBuffer>& cmds) { destroy(cmds.size(), cmds.data()); }
  void destroy(VkCommandBuffer cmd) { destroy(1, &cmd); }

  VkCommandPool getCommandPool() const { return m_commandPool; }

  // Ends command buffer recording and submits to queue, if 'fence' is not
  // VK_NULL_HANDLE, it will be used to signal the completion of the command
  // buffer execution. Does NOT destroy the command buffers! This is not
  // optimal use for queue submission asity may lead to a large number of
  // vkQueueSubmit() calls per frame. . Consider batching submissions up via
  // FencedCommandPools and BatchedSubmission classes down below.
  void submit(size_t count, const VkCommandBuffer* cmds, VkQueue queue, VkFence fence = VK_NULL_HANDLE);
  void submit(size_t count, const VkCommandBuffer* cmds, VkFence fence = VK_NULL_HANDLE);
  void submit(const std::vector<VkCommandBuffer>& cmds, VkFence fence = VK_NULL_HANDLE);

  // Non-optimal usage pattern using wait for idles, avoid in production use.
  // Consider batching submissions up via FencedCommandPools and
  // BatchedSubmission classes down below. Ends command buffer recording and
  // submits to queue, waits for queue idle and destroys cmds.
  void submitAndWait(size_t count, const VkCommandBuffer* cmds, VkQueue queue);
  void submitAndWait(const std::vector<VkCommandBuffer>& cmds, VkQueue queue)
  {
    submitAndWait(cmds.size(), cmds.data(), queue);
  }
  void submitAndWait(VkCommandBuffer cmd, VkQueue queue) { submitAndWait(1, &cmd, queue); }

  // ends and submits to default queue, waits for queue idle and destroys cmds
  void submitAndWait(size_t count, const VkCommandBuffer* cmds) { submitAndWait(count, cmds, m_queue); }
  void submitAndWait(const std::vector<VkCommandBuffer>& cmds) { submitAndWait(cmds.size(), cmds.data(), m_queue); }
  void submitAndWait(VkCommandBuffer cmd) { submitAndWait(1, &cmd, m_queue); }


protected:
  VkDevice      m_device      = VK_NULL_HANDLE;
  VkQueue       m_queue       = VK_NULL_HANDLE;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
};


//--------------------------------------------------------------------------------------------------
/**
  \class nvvk::ScopeCommandBuffer

  nvvk::ScopeCommandBuffer provides a single VkCommandBuffer that lives within the scope
  and is directly submitted and deleted when the scope is left.
  Not recommended for efficiency, since it results in a blocking
  operation, but aids sample writing.

  Example:
  \code{.cpp}
  {
    ScopeCommandBuffer cmd(device, queueFamilyIndex, queue);
    ... do stuff
    vkCmdCopyBuffer(cmd, ...);
  }
  \endcode
*/

class ScopeCommandBuffer : public CommandPool
{
public:
  // if queue is null, uses first queue from familyIndex
  ScopeCommandBuffer(VkDevice device, uint32_t familyIndex, VkQueue queue = VK_NULL_HANDLE)
  {
    CommandPool::init(device, familyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queue);
    m_cmd = createCommandBuffer();
  }

  ~ScopeCommandBuffer() { submitAndWait(m_cmd); }

  operator VkCommandBuffer() const { return m_cmd; };

private:
  VkCommandBuffer m_cmd;
};

//--------------------------------------------------------------------------------------------------
/**
  \classes **nvvk::Ring...**

  In real-time processing, the CPU typically generates commands 
  in advance to the GPU and send them in batches for execution.

  To avoid having the CPU to wait for the GPU'S completion and let it "race ahead"
  we make use of double, or tripple-buffering techniques, where we cycle through
  a pool of resources every frame. We know that those resources are currently 
  not in use by the GPU and can therefore manipulate them directly.
  
  Especially in Vulkan it is the developer's responsibility to avoid such
  access of resources that are in-flight.

  The "Ring" classes cycle through a pool of resources. The default value
  is set to allow two frames in-flight, assuming one fence is used per-frame.
*/

// typically the driver will not let the CPU race ahead more than two frames of GPU
// during swapchain operations.
static const uint32_t DEFAULT_RING_SIZE = 3;
//--------------------------------------------------------------------------------------------------
/**
  #\class nvvk::RingFences

  nvvk::RingFences recycles a fixed number of fences, provides information in which cycle
  we are currently at, and prevents accidental access to a cycle in-flight.

  A typical frame would start by "setCycleAndWait", which waits for the
  requested cycle to be available.
*/

class RingFences
{
public:
  RingFences(RingFences const&) = delete;
  RingFences& operator=(RingFences const&) = delete;

  RingFences() {}
  RingFences(VkDevice device, uint32_t ringSize = DEFAULT_RING_SIZE) { init(device, ringSize); }
  ~RingFences() { deinit(); }

  void init(VkDevice device, uint32_t ringSize = DEFAULT_RING_SIZE);
  void deinit();
  void reset()
  {
    VkDevice device   = m_device;
    uint32_t ringSize = m_cycleSize;
    deinit();
    init(device, ringSize);
  }

  // ensures the availability of the passed cycle
  void setCycleAndWait(uint32_t cycle);
  // get current cycle fence
  VkFence getFence();

  // query current cycle index
  uint32_t getCycleIndex() const { return m_cycleIndex; }
  uint32_t getCycleSize() const { return m_cycleSize; }

private:
  struct Entry
  {
    VkFence fence;
    bool    active;
  };

  uint32_t           m_cycleIndex{0};
  uint32_t           m_cycleSize{0};
  std::vector<Entry> m_fences;
  VkDevice           m_device = VK_NULL_HANDLE;
};
//--------------------------------------------------------------------------------------------------
/**
  #\class nvvk::RingCommandPool

  nvvk::RingCommandPool manages a fixed cycle set of VkCommandBufferPools and
  one-shot command buffers allocated from them.

  The usage of multiple command buffer pools also means we get nice allocation
  behavior (linear allocation from frame start to frame end) without fragmentation.
  If we were using a single command pool over multiple frames, it could fragment easily.

  You must ensure cycle is available manually, typically by keeping in sync
  with ring fences.

  Example:

  \code{.cpp}
  {
    frame++;

    // wait until we can use the new cycle 
    // (very rare if we use the fence at then end once per-frame)
    ringFences.setCycleAndWait( frame );

    // update cycle state, allows recycling of old resources
    ringPool.setCycle( frame );

    VkCommandBuffer cmd = ringPool.createCommandBuffer(...);
    ... do stuff / submit etc...

    VkFence fence = ringFences.getFence();
    // use this fence in the submit
    vkQueueSubmit(...fence..);
  }
  \endcode
*/

class RingCommandPool
{
public:
  RingCommandPool(RingCommandPool const&) = delete;
  RingCommandPool& operator=(RingCommandPool const&) = delete;

  RingCommandPool(VkDevice                 device,
                  uint32_t                 queueFamilyIndex,
                  VkCommandPoolCreateFlags flags    = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                  uint32_t                 ringSize = DEFAULT_RING_SIZE)
  {
    init(device, queueFamilyIndex, flags, ringSize);
  }
  RingCommandPool() {}
  ~RingCommandPool() { deinit(); }

  void init(VkDevice                 device,
            uint32_t                 queueFamilyIndex,
            VkCommandPoolCreateFlags flags    = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            uint32_t                 ringSize = DEFAULT_RING_SIZE);
  void deinit();

  void reset()
  {
    VkDevice                 device           = m_device;
    VkCommandPoolCreateFlags flags            = m_flags;
    uint32_t                 queueFamilyIndex = m_familyIndex;
    uint32_t                 ringSize         = m_cycleSize;
    deinit();
    init(device, queueFamilyIndex, flags, ringSize);
  }

  // call when cycle has changed, prior creating command buffers
  // resets old pools etc.
  void setCycle(uint32_t cycle);

  // ensure proper cycle or frame is set prior these
  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel      level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                      bool                      begin = true,
                                      VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                      const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr);

  // pointer is only valid until next create
  const VkCommandBuffer* createCommandBuffers(VkCommandBufferLevel level, uint32_t count);

protected:
  struct Entry
  {
    VkCommandPool                pool{};
    std::vector<VkCommandBuffer> cmds;
  };

  uint32_t                 m_cycleIndex{0};
  uint32_t                 m_cycleSize{0};
  std::vector<Entry>       m_pools;
  VkDevice                 m_device = VK_NULL_HANDLE;
  VkCommandPoolCreateFlags m_flags{0};
  uint32_t                 m_familyIndex{0};
};

//--------------------------------------------------------------------------------------------------
/**
  \class nvvk::BatchSubmission

  nvvk::BatchSubmission batches the submission arguments of VkSubmitInfo for VkQueueSubmit.

  vkQueueSubmit is a rather costly operation (depending on OS)
  and should be avoided to be done too often (e.g. < 10 per frame). Therefore 
  this utility class allows adding commandbuffers, semaphores etc. and
  submit them later in a batch.

  When using manual locks, it can also be useful to feed commandbuffers
  from different threads and then later kick it off.

  Example

  \code{.cpp}
    // within upload logic
    {
      semTransfer = handleUpload(...);
      // for example trigger async upload on transfer queue here
      vkQueueSubmit(... semTransfer ...);

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
      // enqueue some graphics work for submission
      graphicsSubmission.enqueue(getSceneCmdBuffer());
      graphicsSubmission.enqueue(getUiCmdBuffer());

      graphicsSubmission.execute(frameFence);
    }
  \endcode
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
  BatchSubmission(BatchSubmission const&) = delete;
  BatchSubmission& operator=(BatchSubmission const&) = delete;

  BatchSubmission() {}
  BatchSubmission(VkQueue queue) { init(queue); }

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

  void waitIdle() const;
};

//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::FencedCommandPools

  nvvk::FencedCommandPools container class contains the typical utilities to handle
  command submission. It contains RingFences, RingCommandPool and BatchSubmission
  with a convenient interface.

*/
class FencedCommandPools : protected RingFences, protected RingCommandPool, protected BatchSubmission
{
public:
  FencedCommandPools(FencedCommandPools const&) = delete;
  FencedCommandPools& operator=(FencedCommandPools const&) = delete;

  FencedCommandPools() {}
  ~FencedCommandPools() { deinit(); }

  FencedCommandPools(VkDevice                 device,
                     VkQueue                  queue,
                     uint32_t                 queueFamilyIndex,
                     VkCommandPoolCreateFlags flags    = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                     uint32_t                 ringSize = DEFAULT_RING_SIZE)
  {
    init(device, queue, queueFamilyIndex, flags, ringSize);
  }

  void init(VkDevice                 device,
            VkQueue                  queue,
            uint32_t                 queueFamilyIndex,
            VkCommandPoolCreateFlags flags    = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            uint32_t                 ringSize = DEFAULT_RING_SIZE)
  {
    RingFences::init(device, ringSize);
    RingCommandPool::init(device, queueFamilyIndex, flags, ringSize);
    BatchSubmission::init(queue);
  }

  void deinit()
  {
    RingFences::deinit();
    RingCommandPool::deinit();
    //BatchSubmission::deinit();
  }

  void reset()
  {
    waitIdle();
    RingFences::reset();
    RingCommandPool::reset();
  }

  void enqueue(uint32_t num, const VkCommandBuffer* cmdbuffers) { BatchSubmission::enqueue(num, cmdbuffers); }
  void enqueue(VkCommandBuffer cmdbuffer) { BatchSubmission::enqueue(cmdbuffer); }
  void enqueueSignal(VkSemaphore sem) { BatchSubmission::enqueueSignal(sem); }
  void enqueueWait(VkSemaphore sem, VkPipelineStageFlags flag) { BatchSubmission::enqueueWait(sem, flag); }
  VkResult execute(uint32_t deviceMask = 0) { return BatchSubmission::execute(getFence(), deviceMask); }

  void waitIdle() const { BatchSubmission::waitIdle(); }

  void setCycleAndWait(uint32_t cycle)
  {
    RingFences::setCycleAndWait(cycle);
    RingCommandPool::setCycle(cycle);
  }

  // ensure proper cycle is set prior this
  VkCommandBuffer createCommandBuffer(VkCommandBufferLevel      level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                      bool                      begin = true,
                                      VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                      const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr)
  {
    return RingCommandPool::createCommandBuffer(level, begin, flags, pInheritanceInfo);
  }

  // pointer is only valid until next create
  const VkCommandBuffer* createCommandBuffers(VkCommandBufferLevel level, uint32_t count)
  {
    return RingCommandPool::createCommandBuffers(level, count);
  }

  struct ScopedCmd
  {
    FencedCommandPools* pCmdPools;
    VkCommandBuffer     cmd;

    ScopedCmd(FencedCommandPools& cp)
    {
      pCmdPools = &cp;
      cmd       = cp.createCommandBuffer();
    }
    ~ScopedCmd()
    {
      vkEndCommandBuffer(cmd);
      pCmdPools->enqueue(cmd);
      pCmdPools->execute();
      pCmdPools->waitIdle();
    }

    operator VkCommandBuffer() { return cmd; }
  };
};


}  // namespace nvvk
