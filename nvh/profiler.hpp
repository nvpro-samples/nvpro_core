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


#include <stdint.h>
#include <memory>
#include <stdio.h>
#include <string>
#include <vector>
#include <functional>

#ifdef SUPPORT_NVTOOLSEXT
#define NVTX_STDINT_TYPES_ALREADY_DEFINED
#include "NSight/nvToolsExt.h"
#endif

namespace nvh {
class Profiler
{
  //////////////////////////////////////////////////////////////////////////
  //
  // The Profiler class is designed to measure timed sections.
  // Each section has a cpu and gpu time. Gpu times are typically provided
  // by derived classes for each individual api (e.g. OpenGL, Vulkan etc.).
  //
  // There is functionality to pretty print the sections with their nesting level.
  // Multiple profilers can reference the same database, so one profiler
  // can serve as master that they others contribute to.
  //
  // not thread-safe, must be manually managed

public:
  // if we detect a change in timers (api/name change we trigger a reset after that amount of frames)
  static const uint32_t CONFIG_DELAY   = 8;
  // gpu times are queried after that amount of frames
  static const uint32_t FRAME_DELAY    = 4;
  // by default we start with space for that many begin/end sections per-frame
  static const uint32_t START_SECTIONS = 64;

public:

  typedef uint32_t SectionID;

  class Clock
  {
    // generic utility class for measuring time
    // uses high resolution timer provided by OS
  public:
    Clock();
    double getMicroSeconds() const;

  private:
    double m_frequency;
  };


  //////////////////////////////////////////////////////////////////////////

  // gpu times for a section are queried at "endFrame" with the use of this optional function.
  // It returns true if the queried result was available, and writes the microseconds into gpuTime.
  typedef std::function<bool(SectionID, uint32_t subFrame, double& gpuTime)> gpuTimeProvider_fn;

  // must be called every frame
  void beginFrame();
  void endFrame();

  // sections can be nested, but must be within timeframe
  SectionID beginSection(const char* name, const char* api = nullptr, gpuTimeProvider_fn gpuTimeProvider = nullptr);
  void      endSection(SectionID slot);

  // When a section is used within a loop (same nesting level), and the the same arguments for name and api are
  // passed, we normally average the results of those sections together when printing the stats or using the 
  // getAveraged functions below.
  // Calling the splitter (outside of a section) means we insert a split point that the averaging will not
  // pass.
  void      accumulationSplit();

  class Section
  {
    // helper class for automatic calling of begin/end within a scope
  public:
  Section(Profiler& profiler, const char* name)
    : m_profiler(profiler)
  {
    m_id = profiler.beginSection(name, nullptr, nullptr);
  }
  ~Section() { m_profiler.endSection(m_id); }

  private:
    SectionID m_id;
    Profiler& m_profiler;
  };

  //////////////////////////////////////////////////////////////////////////

  // resets all stats
  void clear();

  // in case averaging should be reset after a few frames (warm-up cache, hide early heavier frames after
  // configuration changes)
  void reset(uint32_t delay = CONFIG_DELAY);

  // pretty print current averaged timers
  void     print(std::string& stats);

  // query functions for current gathered averages
  uint32_t getAveragedFrames(const char* name = NULL) const;
  bool     getAveragedValues(const char* name, double& cpuTimer, double& gpuTimer);
  bool     getAveragedValues(SectionID slot, double& cpuTimer, double& gpuTimer, bool& accumulated);

  inline double getMicroSeconds() const { return m_clock.getMicroSeconds(); }

  //////////////////////////////////////////////////////////////////////////

  // Utility functions for derived classes that provide gpu times.
  // We assume most apis use a big pool of api-specific events/timers,
  // the functions below help manage such pool.

  inline uint32_t getSubFrame() const { return m_data->numFrames % FRAME_DELAY; }
  inline uint32_t getRequiredTimers() const { return (uint32_t)(m_data->entries.size() * FRAME_DELAY * 2); }
  
  static inline uint32_t  getTimerIdx(SectionID slot, uint32_t subFrame, bool begin)
  {
    // must not change order of begin/end
    return slot * (FRAME_DELAY * 2) + subFrame * 2 + (begin ? 0 : 1);
  }

  //////////////////////////////////////////////////////////////////////////

  // if a master is provided we use its database
  // otherwise our own
  Profiler(Profiler* master = nullptr);

  Profiler(uint32_t startSections);
  
private:
  //////////////////////////////////////////////////////////////////////////

  struct Entry
  {
    const char* name = nullptr;
    const char* api  = nullptr;
    gpuTimeProvider_fn gpuTimeProvider = nullptr;

    uint32_t level = 0;
#ifdef SUPPORT_NVTOOLSEXT
    nvtxRangeId_t m_nvrange;
#endif
    double cpuTimes[FRAME_DELAY] = {0};
    double gpuTimes[FRAME_DELAY] = {0};

    uint32_t numTimes = 0;
    double   gpuTime  = 0;
    double   cpuTime  = 0;

    bool splitter    = false;
    bool accumulated = false;
  };

  struct Data
  {
    uint32_t           resetDelay   = 0;
    uint32_t           numFrames    = 0;
    uint32_t           level        = 0;
    uint32_t           frameEntries = 0;
    uint32_t           lastEntries  = 0;
    std::vector<Entry> entries;
  };


  std::shared_ptr<Data> m_data = nullptr;
  Clock                 m_clock;
  
  void grow(uint32_t newsize);
};
}  // namespace nvh

//////////////////////////////////////////////////////////////////////////

namespace nvh {

inline void Profiler::accumulationSplit()
{
  SectionID sec = m_data->frameEntries++;
  if(sec >= m_data->entries.size())
  {
    grow((uint32_t)(m_data->entries.size() * 2));
  }

  m_data->entries[sec].level    = m_data->level;
  m_data->entries[sec].splitter = true;
}

inline Profiler::SectionID Profiler::beginSection(const char* name, const char* api, gpuTimeProvider_fn gpuTimeProvider)
{
  uint32_t subFrame = m_data->numFrames % FRAME_DELAY;
  SectionID     sec = m_data->frameEntries++;

  if(sec >= m_data->entries.size())
  {
    grow((uint32_t)(m_data->entries.size() * 2));
  }

  Entry& entry = m_data->entries[sec];

  if(entry.name != name || entry.api != api)
  {
    entry.name         = name;
    entry.api          = api;
    m_data->resetDelay = CONFIG_DELAY;
  }

  uint32_t level = m_data->level++;
  entry.level    = level;
  entry.splitter = false;
  entry.gpuTimeProvider = gpuTimeProvider;

#ifdef SUPPORT_NVTOOLSEXT
  {
    nvtxEventAttributes_t eventAttrib = {0};
    eventAttrib.version               = NVTX_VERSION;
    eventAttrib.size                  = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType             = NVTX_COLOR_ARGB;

    unsigned char color[4];
    color[0] = 255;
    color[1] = 0;
    color[2] = sec % 2 ? 127 : 255;
    color[3] = 255;

    color[2] -= level * 16;
    color[3] -= level * 16;

    eventAttrib.color         = *(uint32_t*)(color);
    eventAttrib.messageType   = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;
    nvtxRangePushEx(&eventAttrib);
  }
#endif

  entry.cpuTimes[subFrame] = -getMicroSeconds();
  entry.gpuTimes[subFrame] = 0;

  return sec;
}

inline void Profiler::endSection(SectionID sec)
{
  uint32_t subFrame = m_data->numFrames % FRAME_DELAY;
  Entry&   entry    = m_data->entries[sec];

  entry.cpuTimes[subFrame] += getMicroSeconds();

#ifdef SUPPORT_NVTOOLSEXT
  nvtxRangePop();
#endif

  m_data->level--;
}

}  // namespace nvh

#endif
