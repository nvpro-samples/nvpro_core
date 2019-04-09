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

#include "appwindowprofiler_vk.hpp"
#include "contextwindow_vk.hpp"

#include <nvh/misc.hpp>
#include <nvh/nvprint.hpp>

namespace nvvk
{

  void AppWindowProfilerVK::contextInit()
  {
    m_contextWindow.init(&m_contextInfo, this);
    m_profilerVK.init(m_contextWindow.m_context.m_device, m_contextWindow.m_context.m_physicalDevice, m_contextWindow.m_context.m_allocator);
  }

  void AppWindowProfilerVK::contextDeinit()
  {
    m_profilerVK.deinit();
    m_contextWindow.deinit();
  }

  void AppWindowProfilerVK::contextScreenshot( const char* bmpfilename, int width, int height )
  {
    // TODO
    //nvh::saveBMP( bmpfilename, width, height, &data[0] );
  }

  void AppWindowProfilerVK::swapResize(int width, int height)
  {
    m_contextWindow.swapResize(width, height);
  }

  void AppWindowProfilerVK::swapPrepare()
  {
    m_contextWindow.swapPrepare();
  }

  void AppWindowProfilerVK::swapBuffers()
  {
    m_contextWindow.swapBuffers();
  }

  void AppWindowProfilerVK::swapVsync(bool state)
  {
    m_contextWindow.swapVsync(state);
  }

  const char* AppWindowProfilerVK::contextGetDeviceName()
  {
    return m_contextWindow.m_context.m_physicalInfo.properties.deviceName;
  }

}


