/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include "nvh/profiler.hpp"
#include <string>
#include <vulkan/vulkan_core.h>

namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::ProfilerVK

  nvvk::ProfilerVK derives from nvh::Profiler and uses vkCmdWriteTimestamp
  to measure the gpu time within a section.

  If profiler.setLabelUsage(true) was used then it will make use
  of vkCmdDebugMarkerBeginEXT and vkCmdDebugMarkerEndEXT for each
  section so that it shows up in tools like NsightGraphics and renderdoc.
  
  Currently the commandbuffers must support vkCmdResetQueryPool as well.
  
  When multiple queues are used there could be problems with the "nesting"
  of sections. In that case multiple profilers, one per queue, are most
  likely better.


  Example:

  \code{.cpp}
  nvvk::ProfilerVK profiler;
  std::string     profilerStats;

  profiler.init(device, physicalDevice, queueFamilyIndex);
  profiler.setLabelUsage(true); // depends on VK_EXT_debug_utils
  
  while(true)
  {
    profiler.beginFrame();

    ... setup frame ...


    {
      // use the Section class to time the scope
      auto sec = profiler.timeRecurring("draw", cmd);

      vkCmdDraw(cmd, ...);
    }

    ... submit cmd buffer ...

    profiler.endFrame();

    // generic print to string
    profiler.print(profilerStats);

    // or access data directly
    nvh::Profiler::TimerInfo info;
    if( profiler.getTimerInfo("draw", info)) {
      // do some updates
      updateProfilerUi("draw", info.gpu.average);
    }
  }

  \endcode
*/

class ProfilerVK : public nvh::Profiler
{
public:
  // hostReset usage depends on VK_EXT_host_query_reset
  // mandatory for transfer-only queues

  //////////////////////////////////////////////////////////////////////////

  // utility class to call begin/end within local scope
  class Section
  {
  public:
    Section(ProfilerVK& profiler, const char* name, VkCommandBuffer cmd, bool singleShot = false, bool hostReset = false)
        : m_profiler(profiler)
    {
      m_id  = profiler.beginSection(name, cmd, singleShot, hostReset);
      m_cmd = cmd;
    }
    ~Section() { m_profiler.endSection(m_id, m_cmd); }

  private:
    SectionID       m_id;
    VkCommandBuffer m_cmd;
    ProfilerVK&     m_profiler;
  };

  // recurring, must be within beginFrame/endFrame
  Section timeRecurring(const char* name, VkCommandBuffer cmd, bool hostReset = false)
  {
    return Section(*this, name, cmd, false, hostReset);
  }

  // singleShot, results are available after FRAME_DELAY many endFrame
  Section timeSingle(const char* name, VkCommandBuffer cmd, bool hostReset = false)
  {
    return Section(*this, name, cmd, true, hostReset);
  }

  //////////////////////////////////////////////////////////////////////////

  ProfilerVK(nvh::Profiler* masterProfiler = nullptr)
      : Profiler(masterProfiler)
  {
    m_debugName = "nvvk::ProfilerVK:" + std::to_string((uint64_t)this);
  }

  ProfilerVK(VkDevice device, VkPhysicalDevice physicalDevice, nvh::Profiler* masterProfiler = nullptr)
      : Profiler(masterProfiler)
  {
    init(device, physicalDevice);
  }

  ~ProfilerVK() { deinit(); }

  void init(VkDevice device, VkPhysicalDevice physicalDevice, int queueFamilyIndex = 0);
  void deinit();
  void setDebugName(const std::string& name) { m_debugName = name; }

  // enable debug label per section, requires VK_EXT_debug_utils
  void setLabelUsage(bool state);

  SectionID beginSection(const char* name, VkCommandBuffer cmd, bool singleShot = false, bool hostReset = false);
  void      endSection(SectionID slot, VkCommandBuffer cmd);

  bool getSectionTime(SectionID i, uint32_t queryFrame, double& gpuTime);

private:
  void resize();
  bool m_useLabels = false;
#if 0
  bool m_useCoreHostReset = false;
#endif

  VkDevice    m_device          = VK_NULL_HANDLE;
  VkQueryPool m_queryPool       = VK_NULL_HANDLE;
  uint32_t    m_queryPoolSize   = 0;
  float       m_frequency       = 1.0f;
  uint64_t    m_queueFamilyMask = ~0;
  std::string m_debugName;
};
}  // namespace nvvk
