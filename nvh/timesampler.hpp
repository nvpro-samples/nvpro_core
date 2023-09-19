/*
 * Copyright (c) 2013-2023, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2013 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */
//--------------------------------------------------------------------
#pragma once
#include <chrono>
#include <string>
#include <cstdarg>
#include <cassert>
#include "nvprint.hpp"

//-----------------------------------------------------------------------------
/// \struct TimeSampler
/// TimeSampler does time sampling work
//-----------------------------------------------------------------------------
struct TimeSampler
{
  using Clock     = std::chrono::steady_clock;
  using TimePoint = typename Clock::time_point;
  bool      bNonStopRendering;
  int       renderCnt;
  TimePoint start_time, end_time;
  int       timing_counter;
  int       maxTimeSamples;
  int       frameFPS;
  double    frameDT;
  TimeSampler()
  {
    bNonStopRendering = true;
    renderCnt         = 1;
    timing_counter    = 0;
    maxTimeSamples    = 60;
    frameDT           = 1.0 / 60.0;
    frameFPS          = 0;
    start_time = end_time = Clock::now();
  }
  inline double getFrameDT() { return frameDT; }
  inline int    getFPS() { return frameFPS; }
  void          resetSampling(int i = 10) { maxTimeSamples = i; }
  bool          update(bool bContinueToRender, bool* glitch = nullptr)
  {
    if(glitch)
      *glitch = false;
    bool updated = false;


    if((timing_counter >= maxTimeSamples) && (maxTimeSamples > 0))
    {
      timing_counter = 0;
      end_time       = Clock::now();

      // Get delta in seconds
      frameDT = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

      // Linux/OSX etc. TODO
      frameDT /= maxTimeSamples;
#define MAXDT (1.0 / 40.0)
#define MINDT (1.0 / 3000.0)
      if(frameDT < MINDT)
      {
        frameDT = MINDT;
      }
      else if(frameDT > MAXDT)
      {
        frameDT = MAXDT;
        if(glitch)
          *glitch = true;
      }
      frameFPS = (int)(1.0 / frameDT);
      // update the amount of samples to average, depending on the speed of the scene
      maxTimeSamples = (int)(0.15 / (frameDT));
      if(maxTimeSamples > 50)
        maxTimeSamples = 50;
      updated = true;
    }
    if(bContinueToRender || bNonStopRendering)
    {
      if(timing_counter == 0)
        start_time = Clock::now();
      timing_counter++;
    }
    return updated;
    return true;
  }
};


/** 
\struct nvh::Stopwatch
\brief Timer in milliseconds. 

Starts the timer at creation and the elapsed time is retrieved by calling `elapsed()`. 
The timer can be reset if it needs to start timing later in the code execution.

Usage:
````cpp
nvh::Stopwatch sw;
...
LOGI("Elapsed: %f ms\n", sw.elapsed()); // --> Elapsed: 128.157 ms
````
*/
namespace nvh {
struct Stopwatch
{
  Stopwatch() { reset(); }
  void   reset() { startTime = std::chrono::steady_clock::now(); }
  double elapsed()
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count() * 1000.;
  }
  std::chrono::time_point<std::chrono::steady_clock> startTime;
};

// Logging the time spent while alive in a scope.
// Usage: at beginning of a function:
//   auto stimer = ScopedTimer("Time for doing X");
// Nesting timers is handled, but since the time is printed when it goes out of
// scope, printing anything else will break the output formatting.
struct ScopedTimer
{
  ScopedTimer(const std::string& str) { init_(str); }
  ScopedTimer(const char* fmt, ...)
  {
    std::string str(256, '\0');  // initial guess. ideally the first try fits
    va_list     args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);  // make a backup as vsnprintf may consume args1
    int rc = vsnprintf(str.data(), str.size(), fmt, args1);
    if(rc >= 0 && static_cast<size_t>(rc + 1) > str.size())
    {
      str.resize(rc + 1);  // include storage for '\0'
      rc = vsnprintf(str.data(), str.size(), fmt, args2);
    }
    va_end(args1);
    assert(rc >= 0 && "vsnprintf error");
    str.resize(rc >= 0 ? static_cast<size_t>(rc) : 0);
    init_(str);
  }
  void init_(const std::string& str)
  {
    // If nesting timers, break the newline of the previous one
    if(s_openNewline)
    {
      assert(s_nesting > 0);
      LOGI("\n");
    }

    m_manualIndent = !str.empty() && (str[0] == ' ' || str[0] == '-' || str[0] == '|');

    // Add indentation automatically if not already in str.
    if(s_nesting > 0 && !m_manualIndent)
    {
      LOGI("%s", indent().c_str());
    }

    LOGI("%s", str.c_str());
    s_openNewline = str.empty() || str[str.size() - 1] != '\n';
    ++s_nesting;
  }
  ~ScopedTimer()
  {
    --s_nesting;
    // If nesting timers and this is the second destructor in a row, indent and
    // print "Total" as it won't be on the same line.
    if(!s_openNewline && !m_manualIndent)
    {
      LOGI("%s|", indent().c_str());
    }
    else
    {
      LOGI(" ");
    }
    LOGI("-> %.3f ms\n", m_stopwatch.elapsed());
    s_openNewline = false;
  }
  static std::string indent()
  {
    std::string result(static_cast<size_t>(s_nesting * 2), ' ');
    for(int i = 0; i < s_nesting * 2; i += 2)
      result[i] = '|';
    return result;
  }
  nvh::Stopwatch                  m_stopwatch;
  bool                            m_manualIndent = false;
  static inline thread_local int  s_nesting      = 0;
  static inline thread_local bool s_openNewline  = false;
};

}  // namespace nvh
