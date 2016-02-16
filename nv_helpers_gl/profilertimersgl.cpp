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

#include "profilertimersgl.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////

namespace nv_helpers_gl
{
  const char* ProfilerTimersGL::TimerTypeName()
  {
    return "GL ";
  }

  bool ProfilerTimersGL::TimerAvailable( nv_helpers::Profiler::TimerIdx idx )
  {
    GLint available = 0;
    glGetQueryObjectiv(m_queries[idx], GL_QUERY_RESULT_AVAILABLE,&available);
    return !!available;
  }

  void ProfilerTimersGL::TimerSetup( nv_helpers::Profiler::TimerIdx idx )
  {
    glQueryCounter(m_queries[idx],GL_TIMESTAMP);
  }

  unsigned long long ProfilerTimersGL::TimerResult( nv_helpers::Profiler::TimerIdx idxBegin, nv_helpers::Profiler::TimerIdx idxEnd )
  {
    GLuint64 beginTime;
    GLuint64 endTime;
    glGetQueryObjectui64v(m_queries[idxBegin], GL_QUERY_RESULT,&beginTime);
    glGetQueryObjectui64v(m_queries[idxEnd],   GL_QUERY_RESULT,&endTime);

    return endTime - beginTime;
  }

  void ProfilerTimersGL::TimerEnsureSize( unsigned int timers )
  {
    GLuint old = (GLuint)m_queries.size();

    if (old == timers){
      return;
    }

    m_queries.resize( timers, 0);
    GLuint add = timers - old;
    glGenQueries( add, &m_queries[old]);
  }

  void ProfilerTimersGL::init( unsigned int timers )
  {
    TimerEnsureSize(timers);
  }

  void ProfilerTimersGL::deinit()
  {
    if (m_queries.empty()) return;

    glDeleteQueries( (GLuint)m_queries.size(), m_queries.data());
    m_queries.clear();
  }

  void ProfilerTimersGL::TimerFlush()
  {
    glFlush();
  }

}

