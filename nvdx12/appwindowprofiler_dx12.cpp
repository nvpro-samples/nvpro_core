/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include "appwindowprofiler_dx12.hpp"

#include <nvh/misc.hpp>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace nvdx12 {

void AppWindowProfilerDX12::contextInit()
{
  if(!m_context.init(m_contextInfo))
  {
    LOGE("FATAL ERROR: failed to create DX12 context\n");
    exit(-1);
    return;
  }

  HWND hWnd = glfwGetWin32Window(m_internal);
  m_swapChain.init(hWnd, m_context.m_factory, m_context.m_device, m_context.m_commandQueue);
  m_swapChain.update(getWidth(), getHeight());
  m_windowState.m_swapSize[0] = m_swapChain.getWidth();
  m_windowState.m_swapSize[1] = m_swapChain.getHeight();
}

void AppWindowProfilerDX12::contextDeinit()
{
  m_swapChain.waitForGpu();

  m_swapChain.deinit();
  m_context.deinit();
}

void AppWindowProfilerDX12::contextSync()
{
  m_swapChain.waitForGpu();
}

void AppWindowProfilerDX12::swapResize(int width, int height)
{
  if((m_swapChain.getWidth() != width) || (m_swapChain.getHeight() != height))
  {
    m_swapChain.update(width, height);
    m_windowState.m_swapSize[0] = m_swapChain.getWidth();
    m_windowState.m_swapSize[1] = m_swapChain.getHeight();
  }
}

void AppWindowProfilerDX12::swapPrepare()
{
  m_swapChain.moveToNextFrame();
}

void AppWindowProfilerDX12::swapBuffers()
{
  m_swapChain.present();
}

void AppWindowProfilerDX12::swapVsync(bool swapVsync)
{
  m_swapChain.setSyncInterval(swapVsync ? 1 : 0);
}

const char* AppWindowProfilerDX12::contextGetDeviceName()
{
  return nullptr;
}

}  // namespace nvvk
