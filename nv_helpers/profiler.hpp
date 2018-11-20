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

#ifndef NV_PROFILER_INCLUDED
#define NV_PROFILER_INCLUDED


#include <stdio.h>
#include <vector>
#include <string>

#ifdef SUPPORT_NVTOOLSEXT
#include <stdint.h>
#define NVTX_STDINT_TYPES_ALREADY_DEFINED
#    include "NSight/nvToolsExt.h"
#endif

namespace nv_helpers
{
  class Profiler {
  public:
    static const int CONFIG_DELAY = 8;
    static const int FRAME_DELAY  = 4;
    static const int START_SECTIONS = 64;
    static const int START_TIMERS   = START_SECTIONS * FRAME_DELAY * 2;

  public:
    typedef unsigned int Slot;
    typedef unsigned int TimerIdx;

    Profiler();

    class FrameHelper {
    public:
      FrameHelper(Profiler& profiler, double curtime, double printInterval, std::string &stats);
      ~FrameHelper();

      Profiler&   m_profiler;
      bool        m_print;
      std::string &m_stats;
    };

    class Clock {
    public:
      Clock();
      double getMicroSeconds() const;
    private:
      double  m_frequency;
    };


    // The interface must be able to provide getRequiredTimers() many timers
    // prior being used first time.
    class GPUInterface {
    public:
      virtual const char* TimerTypeName () = 0 ;
      virtual bool    TimerAvailable ( TimerIdx idx ) = 0;
      virtual unsigned long long  TimerResult( TimerIdx idxBegin
                                             , TimerIdx idxEnd ) = 0;
      virtual void    TimerEnsureSize( unsigned int timers) = 0;
      // optional usage to flush per section
      virtual void    TimerFlush() {};

      // one of either mechanisms must be implemented
      virtual void    TimerSetup(TimerIdx idx) {};
      // if the api supports streams, queues, cmdbuffers etc. wrap preferred payload here
      virtual void    TimerSetup(TimerIdx idx, void* payload) {};
    };

    class Section {
    public:
      Section(Profiler& profiler, const char* name, GPUInterface* gpuif = 0, bool flush = false, void* payload = nullptr)
        : m_profiler(profiler), m_payload(payload)
      {
        m_slot = profiler.beginSection(name, gpuif, flush, payload);
      }
      ~Section() {
        m_profiler.endSection(m_slot, m_payload);
      }

    private:
      Slot        m_slot;
      Profiler&   m_profiler;
      void*       m_payload;
    };

    unsigned int getRequiredTimers() const;

    void reset(int delay = CONFIG_DELAY);
    void accumulationSplit();

    void init();
    void deinit();
    
    void beginFrame();
    void endFrame();

    void print(std::string &stats);

    unsigned int getAveragedFrames(const char* name=NULL) const;
    bool         getAveragedValues(const char* name, double& cpuTimer, double& gpuTimer);

    double  getMicroSeconds() const {
      return m_clock.getMicroSeconds();
    }

    void setDefaultGPUInterface(GPUInterface *gpuif){
      m_defaultGPUIF = gpuif;
      if(gpuif){
        gpuif->TimerEnsureSize( getRequiredTimers() );
      }
    }

    static inline bool isTimerIdxBegin( TimerIdx idx ){
      return idx % 2 == 0;
    }
    static inline Slot getTimerIdxSlot( TimerIdx idx )
    {
      return idx / (FRAME_DELAY * 2);
    }

    Slot    beginSection(const char* name, GPUInterface* gpuif, bool flush, void* cmd);
    void    endSection(Slot slot, void* cmd);
  private:
    void    grow(unsigned int newsize);

    static inline TimerIdx getTimerIdx(Slot idx, int queryFrame, bool begin)
    {
      return idx * (FRAME_DELAY * 2) + queryFrame * 2 + (begin ? 0 : 1);
    }

    struct Entry {
      const char* name;
      int         level;
#ifdef SUPPORT_NVTOOLSEXT
      nvtxRangeId_t m_nvrange;
#endif
      GPUInterface* gpuif;
      bool          flush;
      double        deltas[FRAME_DELAY];

      unsigned int  numTimes;
      double        gpuTimes;
      double        cpuTimes;

      bool        splitter;
      bool        accumulated;
    };

    Clock               m_clock;
    unsigned int        m_numFrames;
    int                 m_level;
    int                 m_resetDelay;
    unsigned int        m_frameEntries;
    unsigned int        m_lastEntries;
    std::vector<Entry>  m_entries;
    double              m_lastPrint;
    GPUInterface*       m_defaultGPUIF;


  };
}

//////////////////////////////////////////////////////////////////////////

namespace nv_helpers
{
  
  inline void Profiler::accumulationSplit()
  {
    Slot slot = m_frameEntries++;
    if (slot >= m_entries.size()){
      grow((unsigned int)(m_entries.size() * 2));
    }

    m_entries[slot].level = m_level;
    m_entries[slot].splitter = true;
  }

  inline unsigned int Profiler::getRequiredTimers() const
  {
    return (unsigned int)(m_entries.size() * FRAME_DELAY * 2);
  }

  inline Profiler::Slot Profiler::beginSection( const char* name, GPUInterface* gpuifProvided, bool flush, void* payload )
  {
    GPUInterface* gpuif = gpuifProvided ? gpuifProvided : m_defaultGPUIF;
    unsigned int queryFrame = m_numFrames % FRAME_DELAY;
    Slot slot = m_frameEntries++;
    if (slot >= m_entries.size()){
      grow((unsigned int)(m_entries.size() * 2));
    }
    if (gpuif){
      gpuif->TimerEnsureSize( getRequiredTimers() );
    }
 
    if (m_entries[slot].name != name ||
        m_entries[slot].gpuif != gpuif )
    {
      m_entries[slot].name = name;
      m_entries[slot].gpuif = gpuif;
      m_resetDelay = CONFIG_DELAY;
    }

    int level = m_level++;
    m_entries[slot].flush = flush;
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

    m_entries[slot].deltas[queryFrame] = -getMicroSeconds();

    if (gpuif){
      if (payload) {
        gpuif->TimerSetup(getTimerIdx(slot, queryFrame, true), payload);
      }
      else {
        gpuif->TimerSetup(getTimerIdx(slot, queryFrame, true));
      }
    }

    return slot;
  }

  inline void Profiler::endSection( Slot slot, void* payload)
  {
    unsigned int queryFrame = m_numFrames % FRAME_DELAY;
    Entry& entry = m_entries[slot];

    if (entry.gpuif){
      if (payload) {
        entry.gpuif->TimerSetup(getTimerIdx(slot, queryFrame, false), payload);
      }
      else {
        entry.gpuif->TimerSetup(getTimerIdx(slot, queryFrame, false));
      }
      if (entry.flush){
        entry.gpuif->TimerFlush();
      }
    }

    entry.deltas[queryFrame] += getMicroSeconds();

#ifdef SUPPORT_NVTOOLSEXT
    nvtxRangePop();
#endif

    m_level--;
  }

}

#endif
