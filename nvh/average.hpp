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

#include <vector>
namespace nvh {

//--------------------------------------------------------------------------------------------------
// This class will average n numbers over time
//
// Usage: Create a variable to average a value and call update with every new value.
// Ex. nvx::Average<double> avg;
//     for (...)
//        double avg_fps = avg.update(fps);
template <class T>
class Average
{
public:
  Average( int arraySize = 50 )
      : m_arraySize( arraySize )
      , m_valuePerFrame( arraySize )
  {
  }

  T update( T newValue )
  {
    // Calculate the average over all frames
    m_frameAccum += newValue - m_valuePerFrame[m_frameIdx];
    m_valuePerFrame[m_frameIdx] = newValue;
    m_frameIdx                  = ( m_frameIdx + 1 ) % m_arraySize;
    m_average                   = m_frameAccum / static_cast<float>( m_arraySize );
    return m_average;
  }
  const std::vector<T>& values() { return m_valuePerFrame; }
  int                   frameIndex() { return m_frameIdx; }
  int                   size() { return m_arraySize; }
  T                     average() { return m_average; }

private:
  int            m_arraySize  = 50;
  int            m_frameIdx   = 0;
  T              m_frameAccum = 0;
  T              m_average    = 0;
  std::vector<T> m_valuePerFrame;
};


}  // namespace nvh
