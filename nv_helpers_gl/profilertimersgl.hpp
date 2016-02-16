/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Neither the name of its contributors may be used to endorse 
     or promote products derived from this software without specific
     prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/

#ifndef NV_PROFILERGL_INCLUDED
#define NV_PROFILERGL_INCLUDED

#if USE_GLEXTWRAPPER
#include <glextwrapper/glextwrapper.h>
/*
glGenQueries
glDeleteQueries
glQueryCounter
glFlush
glGetQueryObjectiv
*/
#else
#include <GL/glew.h>
#endif

#include <stdio.h>
#include <vector>
#include <string>

#include "../nv_helpers/profiler.hpp"


namespace nv_helpers_gl
{

  class ProfilerTimersGL : public nv_helpers::Profiler::GPUInterface {
  public:
    virtual const char* TimerTypeName ();
    virtual bool        TimerAvailable ( nv_helpers::Profiler::TimerIdx idx );
    virtual void        TimerSetup     ( nv_helpers::Profiler::TimerIdx idx );
    virtual unsigned long long  TimerResult(  nv_helpers::Profiler::TimerIdx idxBegin,
                                              nv_helpers::Profiler::TimerIdx idxEnd );
    virtual void        TimerEnsureSize( unsigned int timers);
    virtual void        TimerFlush();

    void init(unsigned int timers);
    void deinit();

  private:
    std::vector<GLuint> m_queries;
  };
}

#endif
