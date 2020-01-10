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
There are three classes `SingleCommandBuffer`, `ScopeCommandBuffer` and `MultipleCommandBuffers`
that aid command buffer creation and submission.

# class nvvkpp::SingleCommandBuffer

With `SingleCommandBuffer`, you create the the command buffer by calling `createCommandBuffer()` and submit all the work by calling `flushCommandBuffer()`

~~~~ c++
nvvk::SingleCommandBuffer sc(m_device, m_graphicsQueueIndex);
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
  nvvk::ScopeCommandBuffer cmdBuf(m_device, m_graphicQueueIndex);
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


}  // namespace nvvkpp
