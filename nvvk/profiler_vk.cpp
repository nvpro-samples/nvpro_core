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

#include "profiler_vk.hpp"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

namespace nvvk {

void ProfilerVK::init(VkDevice device, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* allocator)
{
  m_device = device;
  m_allocator = allocator;

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);

  m_frequency = properties.limits.timestampPeriod;

  resize();
}

void ProfilerVK::deinit()
{
  vkDestroyQueryPool(m_device, m_queryPool, m_allocator);
}

void ProfilerVK::resize()
{
  if (getRequiredTimers() < m_queryPoolSize) return;

  if (m_queryPool) {
    // not exactly efficient, but when timers changed a lot, we have a slow frame anyway
    // cleaner would be allocating more pools
    vkDeviceWaitIdle(m_device);
    vkDestroyQueryPool(m_device, m_queryPool, m_allocator);
  }

  VkQueryPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
  create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
  create_info.queryCount = getRequiredTimers();
  m_queryPoolSize = create_info.queryCount;

  VkResult res = vkCreateQueryPool(m_device, &create_info, m_allocator, &m_queryPool);
  assert(res == VK_SUCCESS);
}

nvh::Profiler::SectionID ProfilerVK::beginSection(const char* name, VkCommandBuffer cmd)
{
  nvh::Profiler::gpuTimeProvider_fn fnProvider = [&](SectionID i, uint32_t queryFrame, double& gpuTime) {
    uint32_t idxBegin = getTimerIdx(i, queryFrame, true);
    uint32_t idxEnd   = getTimerIdx(i, queryFrame, false);

    uint64_t times[2];
    VkResult result = vkGetQueryPoolResults(m_device, m_queryPool, idxBegin, 2, sizeof(uint64_t) * 2, times, 0, VK_QUERY_RESULT_64_BIT);
   
    if(result == VK_SUCCESS)
    {
      gpuTime = (double(times[1] - times[0]) * double(m_frequency)) / double(1000);
      return true;
    }
    else
    {
      return false;
    }
  };


  SectionID slot = Profiler::beginSection(name, "VK ", fnProvider);

  if (getRequiredTimers() > m_queryPoolSize) {
    resize();
  }

  uint32_t idx = getTimerIdx(slot, getSubFrame(), true);
  // clear begin and end
  vkCmdResetQueryPool(cmd, m_queryPool, idx, 2);  // not ideal to do this per query
  vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_queryPool, idx);

  return slot;
}

void ProfilerVK::endSection(SectionID slot, VkCommandBuffer cmd)
{
  uint32_t idx = getTimerIdx(slot, getSubFrame(), false);
  vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_queryPool, idx);

  Profiler::endSection(slot);
}


}  // namespace nvvk
