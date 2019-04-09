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

#pragma once

#include <nvh/profiler.hpp>
#include <vulkan/vulkan.h>

namespace nvvk {

class ProfilerVK : public nvh::Profiler
{
public:
  ProfilerVK(nvh::Profiler* masterProfiler = nullptr)
      : Profiler(masterProfiler)
  {
  }

  class Section
  {
  public:
    Section(ProfilerVK& profiler, const char* name, VkCommandBuffer cmd)
        : m_profiler(profiler)
    {
      m_id = profiler.beginSection(name, cmd);
      m_cmd = cmd;
    }
    ~Section() { m_profiler.endSection(m_id, m_cmd); }

  private:
    SectionID       m_id;
    VkCommandBuffer m_cmd;
    ProfilerVK&     m_profiler;
  };


  void init(VkDevice device, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks *pAllocator = nullptr);
  void deinit();

  SectionID beginSection(const char* name, VkCommandBuffer cmd);
  void      endSection(SectionID slot, VkCommandBuffer cmd);

private:
  void resize();

  VkDevice                     m_device        = VK_NULL_HANDLE;
  const VkAllocationCallbacks* m_allocator     = nullptr;
  VkQueryPool                  m_queryPool     = VK_NULL_HANDLE;
  uint32_t                     m_queryPoolSize = 0;
  float                        m_frequency     = 1.0f;
};
}  // namespace nvvk

