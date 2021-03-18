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

#ifndef NV_WINDOWPROFILER_GL_INCLUDED
#define NV_WINDOWPROFILER_GL_INCLUDED

#include <nvh/appwindowprofiler.hpp>

#include <nvdx12/context_dx12.hpp>
#include <nvdx12/swapchain_dx12.hpp>


namespace nvdx12 {

class AppWindowProfilerDX12 : public nvh::AppWindowProfiler
{
public:
  AppWindowProfilerDX12(bool singleThreaded = true, bool doSwap = true)
      : nvh::AppWindowProfiler(singleThreaded, doSwap)
  {
  }

  ContextCreateInfo m_contextInfo;
  Context           m_context;
  SwapChain         m_swapChain;

  int run(const std::string& name, int argc, const char** argv, int width, int height)
  {
    return AppWindowProfiler::run(name, argc, argv, width, height, false);
  }

  virtual void        contextInit() override;
  virtual void        contextDeinit() override;
  virtual void        contextSync() override;
  virtual const char* contextGetDeviceName() override;

  virtual void        swapResize(int width, int height) override;
  virtual void        swapPrepare() override;
  virtual void        swapBuffers() override;
  virtual void        swapVsync(bool state) override;
  
};
}  // namespace nvvk


#endif
