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

#include "profiler.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(__linux__)
#include <sys/time.h>
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


//////////////////////////////////////////////////////////////////////////

namespace nvh {

Profiler::Profiler(Profiler* master)
{
  m_data = master ? master->m_data : std::shared_ptr<Data>(new Data);
  grow(START_SECTIONS);
}

 Profiler::Profiler(uint32_t startSections)
 {
   m_data = std::shared_ptr<Data>(new Data);
   grow(startSections);
 }

void Profiler::beginFrame()
{
  m_data->frameEntries = 0;
  m_data->level        = 0;
}

void Profiler::endFrame()
{
  if(m_data->frameEntries != m_data->lastEntries)
  {
    m_data->lastEntries = m_data->frameEntries;
    m_data->resetDelay  = CONFIG_DELAY;
  }

  if(m_data->resetDelay)
  {
    m_data->resetDelay--;
    for(uint32_t i = 0; i < m_data->entries.size(); i++)
    {

      m_data->entries[i].numTimes = 0;
      m_data->entries[i].cpuTime  = 0;
      m_data->entries[i].gpuTime  = 0;
    }
    m_data->numFrames = 0;
  }

  if(m_data->numFrames > FRAME_DELAY)
  {
    for(uint32_t i = 0; i < m_data->frameEntries; i++)
    {
      Entry& entry = m_data->entries[i];

      if(entry.splitter)
        continue;

      uint32_t queryFrame = (m_data->numFrames + 1) % FRAME_DELAY;
      bool     available  = entry.api == nullptr || entry.gpuTimeProvider(i, queryFrame, entry.gpuTimes[queryFrame]);

      if(available)
      {
        entry.gpuTime += entry.gpuTimes[queryFrame];
        entry.cpuTime += entry.cpuTimes[queryFrame];
        entry.numTimes++;
      }
    }
  }

  m_data->numFrames++;
}


void Profiler::grow(uint32_t newsize)
{
  size_t oldsize = m_data->entries.size();

  if(oldsize == newsize)
  {
    return;
  }

  m_data->entries.resize(newsize);
}

void Profiler::clear()
{
  m_data->entries.clear();
}

void Profiler::reset(uint32_t delay)
{
  m_data->resetDelay = delay;
}

static std::string format(const char* msg, ...)
{
  std::size_t const STRING_BUFFER(8192);
  char              text[STRING_BUFFER];
  va_list           list;

  if(msg == 0)
    return std::string();

  va_start(list, msg);
  vsprintf(text, msg, list);
  va_end(list);

  return std::string(text);
}

bool Profiler::getAveragedValues(SectionID i, double& cpuTimer, double& gpuTimer, bool& accumulated)
{
  Entry& entry = m_data->entries[i];

  if(!entry.numTimes || entry.accumulated)
  {
    return false;
  }

  double gpu   = entry.gpuTime / double(entry.numTimes);
  double cpu   = entry.cpuTime / double(entry.numTimes);
  bool   found = false;
  for(uint32_t n = i + 1; n < m_data->lastEntries; n++)
  {
    Entry& otherentry = m_data->entries[n];
    if(otherentry.name == entry.name && otherentry.level == entry.level && otherentry.api == entry.api && !otherentry.accumulated)
    {
      found = true;
      gpu += otherentry.gpuTime / double(otherentry.numTimes);
      cpu += otherentry.cpuTime / double(otherentry.numTimes);
      otherentry.accumulated = true;
    }

    if(otherentry.splitter && otherentry.level <= entry.level)
      break;
  }

  cpuTimer    = cpu;
  gpuTimer    = gpu;
  accumulated = found;

  return true;
}

bool Profiler::getAveragedValues(const char* name, double& cpuTimer, double& gpuTimer)
{
  for(uint32_t i = 0; i < m_data->lastEntries; i++)
  {
    Entry& entry      = m_data->entries[i];
    entry.accumulated = false;
  }

  for(uint32_t i = 0; i < m_data->lastEntries; i++)
  {
    Entry& entry = m_data->entries[i];

    if(strcmp(name, entry.name))
      continue;

    bool accumulated = false;
    return getAveragedValues(i, cpuTimer, gpuTimer, accumulated);
  }

  return false;
}

void Profiler::print(std::string& stats)
{
  stats.clear();

  bool hadtimers = false;

  for(uint32_t i = 0; i < m_data->lastEntries; i++)
  {
    Entry& entry      = m_data->entries[i];
    entry.accumulated = false;
  }

  for(uint32_t i = 0; i < m_data->lastEntries; i++)
  {
    static const char* spaces = "        ";  // 8
    Entry&             entry  = m_data->entries[i];
    uint32_t           level  = 7 - (entry.level > 7 ? 7 : entry.level);

    double gpu         = 0;
    double cpu         = 0;
    bool   accumulated = false;

    if(!getAveragedValues(i, cpu, gpu, accumulated))
      continue;

    hadtimers = true;

    const char* gpuname = entry.api ? entry.api : "N/A";

    if(accumulated)
    {
      stats += format("%sTimer %s;\t %s %6d; CPU %6d; (microseconds, accumulated loop)\n", &spaces[level], entry.name,
                      gpuname, (uint32_t)(gpu), (uint32_t)(cpu));
    }
    else
    {
      stats += format("%sTimer %s;\t %s %6d; CPU %6d; (microseconds, avg %d)\n", &spaces[level], entry.name, gpuname,
                      (uint32_t)(gpu), (uint32_t)(cpu), (uint32_t)entry.numTimes);
    }
  }
}

uint32_t Profiler::getAveragedFrames(const char* name) const
{
  if(m_data->entries.empty())
  {
    return 0;
  }
  if(name == nullptr)
  {
    return m_data->entries[0].numTimes;
  }
  for(uint32_t i = 0; i < m_data->lastEntries; i++)
  {
    const Entry& entry = m_data->entries[i];
    if(!strcmp(name, entry.name))
    {
      return entry.numTimes;
    }
  }
  return 0;
}

Profiler::Clock::Clock()
{
  m_frequency = 1;
#ifdef _WIN32
  LARGE_INTEGER sysfrequency;
  if(QueryPerformanceFrequency(&sysfrequency))
  {
    m_frequency = (double)sysfrequency.QuadPart;
  }
  else
  {
    m_frequency = 1;
  }
#endif
}

double Profiler::Clock::getMicroSeconds() const
{
#ifdef _WIN32
  LARGE_INTEGER time;
  if(QueryPerformanceCounter(&time))
  {
    return (double(time.QuadPart) / m_frequency) * 1000000.0;
  }
#elif defined(__linux__)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return double(tv.tv_sec) * 1000000 + double(tv.tv_usec);
#endif
  return 0;
}
}  // namespace nvh
