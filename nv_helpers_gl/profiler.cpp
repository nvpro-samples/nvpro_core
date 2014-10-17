/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#include "profiler.hpp"

#ifdef _WIN32
#define NOMINMAX 
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif


//////////////////////////////////////////////////////////////////////////

namespace nv_helpers
{

  Profiler::Profiler()
    : m_frameEntries(0)
    , m_numFrames(0)
    , m_resetDelay(0)
    , m_frequency(1)
    , m_level(0)
  {

  }

  void Profiler::beginFrame()
  {
    m_frameEntries = 0;
    m_level = 0;
  }

  void Profiler::endFrame()
  {
    if (m_frameEntries != m_lastEntries) {
      m_lastEntries = m_frameEntries;
      m_resetDelay = CONFIG_DELAY;
    }

    if (m_resetDelay) {
      m_resetDelay--;
      for (size_t i = 0; i < m_entries.size(); i++){
        
        m_entries[i].numTimes = 0;
        m_entries[i].cpuTimes = 0;
        m_entries[i].gpuTimes = 0;
      }
      m_numFrames = 0;
    }

    if (m_numFrames > FRAME_DELAY){
      for (size_t i = 0; i < m_frameEntries; i++){
        Entry& entry = m_entries[i];

        if (entry.splitter) continue;

        GLint available = 0;
        GLuint queryFrame = (m_numFrames + 1) % FRAME_DELAY;
        glGetQueryObjectiv(entry.queries[queryFrame + FRAME_DELAY], GL_QUERY_RESULT_AVAILABLE,&available);

        if (available) {
          GLuint64 beginTime;
          GLuint64 endTime;
          glGetQueryObjectui64v(entry.queries[queryFrame], GL_QUERY_RESULT,&beginTime);
          glGetQueryObjectui64v(entry.queries[queryFrame + FRAME_DELAY], GL_QUERY_RESULT,&endTime);

          // nanoseconds to microseconds
          GLuint64 gpuNano = endTime - beginTime;
          double gpu = double(gpuNano) / 1000.0;
          entry.gpuTimes += gpu;
          entry.cpuTimes += entry.deltas[queryFrame];
          entry.numTimes ++;
        }
      }
    }

    m_numFrames++;
  }


  void Profiler::grow(size_t newsize)
  {
    size_t oldsize = m_entries.size();
    assert(newsize > oldsize);

    m_entries.resize(newsize);
    for (size_t i = oldsize; i < newsize; i++){
      Entry &entry = m_entries[i];
      glGenQueries(2 * FRAME_DELAY, &entry.queries[0]);
      entry.name = NULL;
    }
  }

  void Profiler::init()
  {
#ifdef _WIN32
    LARGE_INTEGER sysfrequency;
    if (QueryPerformanceFrequency(&sysfrequency)){
      m_frequency = (double)sysfrequency.QuadPart;
    }
    else{
      m_frequency = 1;
    }
#endif

    grow(START_SECTIONS);
  }

  void Profiler::deinit()
  {
    for (size_t i = 0; i < m_entries.size(); i++){
      Entry &entry = m_entries[i];
      glDeleteQueries(2 * FRAME_DELAY, &entry.queries[0]);
      entry.name = NULL;
    }
  }

  void Profiler::reset(int delay)
  {
    m_resetDelay = delay;
  }

  std::string format(const char* msg, ...)
  {
    std::size_t const STRING_BUFFER(8192);
    char text[STRING_BUFFER];
    va_list list;

    if(msg == 0)
      return std::string();

    va_start(list, msg);
    vsprintf(text, msg, list);
    va_end(list);

    return std::string(text);
  }

  void Profiler::print( std::string &stats)
  {
    stats.clear();

    bool hadtimers = false;

    for (size_t i = 0; i < m_lastEntries; i++){
      Entry &entry = m_entries[i];
      entry.accumulated = false;
    }

    for (size_t i = 0; i < m_lastEntries; i++){
      static const char* spaces = "        "; // 8
      Entry &entry = m_entries[i];
      int level = 7 - (entry.level > 7 ? 7 : entry.level);

      if (!entry.numTimes || entry.accumulated) continue;

      hadtimers = true;

      double gpu = entry.gpuTimes/entry.numTimes;
      double cpu = entry.cpuTimes/entry.numTimes;
      bool found = false;
      for (size_t n = i+1; n < m_lastEntries; n++){
        Entry &otherentry = m_entries[n];
        if (otherentry.name == entry.name && 
          otherentry.level == entry.level &&
          !otherentry.accumulated
          )
        {
          found = true;
          gpu += otherentry.gpuTimes/otherentry.numTimes;
          cpu += otherentry.cpuTimes/otherentry.numTimes;
          otherentry.accumulated = true;
        }

        if (otherentry.splitter && otherentry.level <= entry.level) break;
      }

      if (found){
        stats += format("%sTimer %s;\t GL %6d; CPU %6d; (microseconds, accumulated loop)\n",&spaces[level], entry.name, (unsigned int)(gpu), (unsigned int)(cpu));
      }
      else {
        stats += format("%sTimer %s;\t GL %6d; CPU %6d; (microseconds, avg %d)\n",&spaces[level],entry.name, (unsigned int)(gpu), (unsigned int)(cpu), (unsigned int)entry.numTimes);
      }
    }
  }

  double Profiler::getMicroSeconds()
  {
#ifdef _WIN32
    LARGE_INTEGER time;
    if (QueryPerformanceCounter(&time)){
      return (double(time.QuadPart) / m_frequency)*1000000.0;
    }
#endif
    return 0;
  }

  Profiler::FrameHelper::FrameHelper( Profiler& profiler, double curtime, double printInterval, std::string& stats )
    : m_profiler(profiler)
    , m_stats(stats)
  {
    m_print = ((curtime - m_profiler.m_lastPrint) > printInterval);
    if (m_print){
      m_profiler.m_lastPrint = curtime;
    }
    m_profiler.beginFrame();
  }

  Profiler::FrameHelper::~FrameHelper()
  {
    m_profiler.endFrame();
    if (m_print){
      m_profiler.print(m_stats);
      m_profiler.reset(1);
    }
  }

}

