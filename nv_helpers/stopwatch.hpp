/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
*/ //--------------------------------------------------------------------

#pragma once

#include <chrono>

namespace nv_helpers {


//------------------------------------------------------------------------
// Stopwatch class for statistics.
//
class Stopwatch
{
public:
  // Constructor, initializes the time module member
  Stopwatch() = default;

  // Start the counter
  inline void start();

  // Start the counter and reset the counter
  inline void startNew();

  // Stop the counter
  inline void stop();

  // Reset the counter
  inline void reset();

  // Get elapsed time
  // return: elapsed time
  inline double elapsed() const;

  inline bool is_running() const;

private:
  double get_time();

  double m_ctime      = 0;  // cumulative time
  double m_time       = 0;  // start time
  bool   m_is_running = false;
};


//------------------------------------------------------------------------
// Start the counter
//
inline void Stopwatch::start()
{
  m_time       = get_time();
  m_is_running = true;
}
inline void Stopwatch::startNew()
{
  m_ctime      = 0;
  m_time       = get_time();
  m_is_running = true;
}


//------------------------------------------------------------------------
// Stop the counter
//
inline void Stopwatch::stop()
{
  m_ctime += get_time() - m_time;
  m_is_running = false;
}

//------------------------------------------------------------------------
// Reset the counter
//
inline void Stopwatch::reset()
{
  m_ctime = 0;
}

//------------------------------------------------------------------------
// Get elapsed time in milliseconds.
//
inline double Stopwatch::elapsed() const
{
  return m_ctime;
}

//-----------------------------------------------------------------------------
//
//
inline bool Stopwatch::is_running() const
{
  return m_is_running;
}


//--------------------------------------------------------------------------------------------------
//
//
inline double Stopwatch::get_time()
{
  auto now( std::chrono::system_clock::now() );
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>( duration ).count() / 1000.0;
}

}  // namespace nv_helpers
