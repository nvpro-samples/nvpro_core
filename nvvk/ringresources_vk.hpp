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
#include <vulkan/vulkan.h>

namespace nvvk {

  /*
    In real-time processing, the CPU typically generates commands 
    in advance to the GPU and send them in batches for exeuction.

    To avoid having he CPU to wait for the GPU'S completion and let it "race ahead"
    we make use of double, or tripple-buffering techniques, where we cycle through
    a set of resources every frame. We know that those resources are currently 
    not in use by the GPU and can therefore manipulate them directly.
  
    Especially in Vulkan it is the developers responsibility to avoid such
    access of resources inflight.
  */

  // FIXME template cycle count?
  static const uint32_t MAX_RING_FRAMES = 3;

  class RingFences
  {
    /*
      Recycles a fixed number of fences, provides information in which cycle
      we are currently at, and prevents accidental access to a cycle inflight.

      A typical frame would start by waiting for older cycle's completion ("wait")
      and be ended by "advanceCycle".

      Safely index other resources, for example ring buffers using the "getCycleIndex"
      for the current frame.
    */


  public:
    void init(VkDevice device, const VkAllocationCallbacks* allocator = nullptr);
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
    const VkAllocationCallbacks* m_allocator;
  };

  class RingCmdPool
  {
    /*
      Manages a fixed cycle set of VkCommandBufferPools and
      one-shot command buffers allocated from them.

      Every cycle a different command buffer pool is used for
      providing the command buffers. Command buffers are automatically
      deleted after a full cycle (MAX_RING_FRAMES) has been completed.

      The usage of multiple command buffer pools also means we get nice allocation
      behavior (linear allocation from frame start to frame end) without fragmentation.
      If we were using a single command pool, it would fragment easily.
    
    */

  public:
    void init(VkDevice             device,
      uint32_t                     queueFamilyIndex,
      VkCommandPoolCreateFlags     flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      const VkAllocationCallbacks* allocator = nullptr);
    void deinit();
    void reset(VkCommandPoolResetFlags flags = 0);

    // call once per cycle prior creating command buffers
    // resets old pools etc.
    void setCycle(uint32_t cycleIndex);

    // ensure proper cycle is set prior this
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level);

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
    const VkAllocationCallbacks* m_allocator;
    uint32_t                     m_index;
    uint32_t                     m_dirty;
  };

}
