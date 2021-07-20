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


#ifndef NV_WINDOWPROFILER_GL_INCLUDED
#define NV_WINDOWPROFILER_GL_INCLUDED

#include <nvh/appwindowprofiler.hpp>

#include <nvvk/context_vk.hpp>
#include <nvvk/profiler_vk.hpp>
#include <nvvk/swapchain_vk.hpp>


namespace nvvk {

//////////////////////////////////////////////////////////////////////////
/**
  \class nvvk::AppWindowProfilerVK

  nvvk::AppWindowProfilerVK derives from nvh::AppWindowProfiler
  and overrides the context and swapbuffer functions.
  The nvh class itself provides several utilities and 
  command line options to run automated benchmarks etc.
  
  To influence the vulkan instance/device creation modify 
  `m_contextInfo` prior running AppWindowProfiler::run,
  which triggers instance, device, window, swapchain creation etc.

  The class comes with a nvvk::ProfilerVK instance that references the 
  AppWindowProfiler::m_profiler's data.
*/

#define NV_PROFILE_VK_SECTION(name, cmd) const nvvk::ProfilerVK::Section _tempTimer(m_profilerVK, name, cmd)
#define NV_PROFILE_VK_SPLIT() m_profilerVK.accumulationSplit()

class AppWindowProfilerVK : public nvh::AppWindowProfiler
{
public:
  AppWindowProfilerVK(bool singleThreaded = true)
      : nvh::AppWindowProfiler(singleThreaded)
      , m_profilerVK(&m_profiler)
  {
  }

  bool              m_swapVsync = false;
  ContextCreateInfo m_contextInfo{};
  Context           m_context{};
  SwapChain         m_swapChain{};
  VkSurfaceKHR      m_surface{};
  ProfilerVK        m_profilerVK{};

  int run(const std::string& name, int argc, const char** argv, int width, int height)
  {
    return AppWindowProfiler::run(name, argc, argv, width, height, false);
  }

  virtual void        contextInit() override;
  virtual void        contextDeinit() override;
  virtual void        contextSync() override;
  virtual const char* contextGetDeviceName() override;

  virtual void swapResize(int width, int height) override;
  virtual void swapPrepare() override;
  virtual void swapBuffers() override;
  virtual void swapVsync(bool state) override;
};
}  // namespace nvvk


#endif
