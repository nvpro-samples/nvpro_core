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

#ifndef NV_PROFILER_INCLUDED
#define NV_PROFILER_INCLUDED

#include <GL/glew.h>
#include <stdio.h>
#include <vector>

#ifndef NV_TIMER_FLUSH
#define NV_TIMER_FLUSH 0
#endif

#ifdef SUPPORT_NVTOOLSEXT

#if _MSC_VER >= 1600 
#include <stdint.h>
#else 
typedef unsigned __int8 uint8_t;
typedef __int8 int8_t;
typedef unsigned __int16 uint16_t;
typedef __int16 int16_t;
typedef unsigned __int32 uint32_t;
typedef __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#endif
#define NVTX_STDINT_TYPES_ALREADY_DEFINED
#include <nvToolsExt.h>
#endif

namespace nv_helpers_gl
{
  class Profiler {
    static const int CONFIG_DELAY = 16;
    static const int FRAME_DELAY  = 8;
    static const int START_SECTIONS = 64;

  public:
    typedef unsigned int Slot;

    Profiler();

    class FrameHelper {
    public:
      FrameHelper(Profiler& profiler, double curtime, double printInterval, std::string &stats);
      ~FrameHelper();

      Profiler&   m_profiler;
      bool        m_print;
      std::string &m_stats;
    };

    class Section {
    public:
      Section(Profiler& profiler, const char* name)
        : m_profiler(profiler)
      {
        m_slot = profiler.beginSection(name);
      }
      ~Section() {
        m_profiler.endSection(m_slot);
      }

    private:
      Slot        m_slot;
      Profiler&   m_profiler;
    };

    void reset(int delay = CONFIG_DELAY);
    void accumulationSplit();

    void init();
    void deinit();
    
    void beginFrame();
    void endFrame();

    void print(std::string &stats);

  private:
    double  getMicroSeconds();
    
    Slot    beginSection( const char* name);
    void    endSection  ( Slot slot);
    void    grow(size_t newsize);

    struct Entry {
      const char* name;
      int         level;
#ifdef SUPPORT_NVTOOLSEXT
      nvtxRangeId_t m_nvrange;
#endif
      GLuint      queries[FRAME_DELAY * 2];
      double      deltas[FRAME_DELAY];

      double      numTimes;
      double      gpuTimes;
      double      cpuTimes;

      bool        splitter;
      bool        accumulated;
    };

    double              m_frequency;
    unsigned int        m_numFrames;
    int                 m_level;
    int                 m_resetDelay;
    unsigned int        m_frameEntries;
    unsigned int        m_lastEntries;
    std::vector<Entry>  m_entries;
    double              m_lastPrint;


  };
}

//////////////////////////////////////////////////////////////////////////

namespace nv_helpers_gl
{

  inline void Profiler::accumulationSplit()
  {
    Slot slot = m_frameEntries++;
    if (slot >= m_entries.size()){
      grow( m_entries.size() * 2);
    }

    m_entries[slot].level = m_level;
    m_entries[slot].splitter = true;
  }

  inline Profiler::Slot Profiler::beginSection( const char* name )
  {
    GLuint queryFrame = m_numFrames % FRAME_DELAY;
    Slot slot = m_frameEntries++;
    if (slot >= m_entries.size()){
      grow( m_entries.size() * 2);
    }
 
    if (m_entries[slot].name != name){
      m_entries[slot].name = name;
      m_resetDelay = CONFIG_DELAY;
    }

    int level = m_level++;
    m_entries[slot].level = level;
    m_entries[slot].splitter = false;

#ifdef SUPPORT_NVTOOLSEXT
    {
      nvtxEventAttributes_t eventAttrib = {0};
      eventAttrib.version = NVTX_VERSION;
      eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
      eventAttrib.colorType = NVTX_COLOR_ARGB;

      unsigned char color[4];
      color[0] = 255;
      color[1] = 0;
      color[2] = slot % 2 ? 127 : 255;
      color[3] = 255;
      
      color[2] -= level * 16;
      color[3] -= level * 16;

      eventAttrib.color = *(uint32_t*)(color);
      eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
      eventAttrib.message.ascii = name;
      nvtxRangePushEx(&eventAttrib);
    }
#endif

    glQueryCounter(m_entries[slot].queries[queryFrame],GL_TIMESTAMP);
    m_entries[slot].deltas[queryFrame] = -getMicroSeconds();

    return slot;
  }

  inline void Profiler::endSection( Slot slot )
  {
    GLuint queryFrame = m_numFrames % FRAME_DELAY;

    m_entries[slot].deltas[queryFrame] += getMicroSeconds();
    glQueryCounter(m_entries[slot].queries[queryFrame + FRAME_DELAY],GL_TIMESTAMP);
#if NV_TIMER_FLUSH
    glFlush();
#endif

#ifdef SUPPORT_NVTOOLSEXT
    nvtxRangePop();
#endif

    m_level--;
  }

}

#endif
