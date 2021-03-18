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

#ifndef NV_WINDOWPROFILER_DX_INCLUDED
#define NV_WINDOWPROFILER_DX_INCLUDED

#include <nvh/appwindowprofiler.hpp>
#include "profilertimers_dx11.hpp"

namespace nvdx11
{

  class AppWindowProfilerDX11 : public nvh::AppWindowProfiler {
  public:

    AppWindowProfilerDX11(bool singleThreaded = true, bool doSwap = true) 
      : nvh::AppWindowProfiler(NVPWindow::WINDOW_API_DX11, singleThreaded, doSwap)
    {

    }
    
    nvdx11::ProfilerTimersDX m_dxtimers;
    ContextFlagsDX11                  m_cflags;

    virtual const ContextFlagsBase* preWindowContext( int apiMajor, int apiMinor );
    virtual void  postWindow();
    virtual void  postEnd();
    virtual void  dumpScreenshot( const char* bmpfilename, int width, int height );
  };
}


#endif


