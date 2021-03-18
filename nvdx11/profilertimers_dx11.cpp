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

#include "profilertimers_dx11.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////

namespace nvdx11
{
  const char* ProfilerTimersDX::TimerTypeName()
  {
    return "GPU ";
  }

  bool ProfilerTimersDX::TimerAvailable( nvh::Profiler::TimerIdx idx )
  {
    return true;
  }

  void ProfilerTimersDX::TimerSetup( nvh::Profiler::TimerIdx idx )
  {
    bool begin = nvh::Profiler::isTimerIdxBegin(idx);
    nvh::Profiler::Slot slot = nvh::Profiler::getTimerIdxSlot( idx );

    if ( begin ){
      m_context->Begin(m_queriesDisjoint[slot]);
    }

    m_context->End(m_queries[idx]);

    if (!begin){
      m_context->End( m_queriesDisjoint[slot] );
    }
  }

  unsigned long long ProfilerTimersDX::TimerResult( nvh::Profiler::TimerIdx idxBegin, nvh::Profiler::TimerIdx idxEnd )
  {
    UINT64 beginTime;
    UINT64 endTime;
    
    while (m_context->GetData( m_queries[idxBegin], &beginTime, sizeof( beginTime ), 0 ) != S_OK);
    while (m_context->GetData( m_queries[idxEnd], &endTime, sizeof( endTime ), 0 ) != S_OK);

    nvh::Profiler::Slot slot = nvh::Profiler::getTimerIdxSlot( idxBegin );

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
    while (m_context->GetData( m_queriesDisjoint[slot], &disjointData, sizeof( disjointData ), 0 ) != S_OK);


    if (disjointData.Disjoint == FALSE)
    {
      double delta = double(endTime - beginTime);
      double frequency = double(disjointData.Frequency);
      double time = (delta / frequency) * 1000000.0f;

      return UINT64(time);
    }

    return 0;
  }

  void ProfilerTimersDX::TimerEnsureSize( unsigned int timers )
  {
    size_t old = m_queries.size();

    if (old == timers){
      return;
    }

    m_queries.resize( timers, 0);
    m_queriesDisjoint.resize( timers, 0);
    size_t add = size_t(timers) - old;
    for (size_t i=0; i < add; i++){
      D3D11_QUERY_DESC desc;
      ID3D11Query* query;
      desc.MiscFlags = 0;
      desc.Query = D3D11_QUERY_TIMESTAMP;

      m_device->CreateQuery(&desc,&query);
      m_queries[old + i] = query;

      desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
      m_device->CreateQuery(&desc,&query);
      m_queriesDisjoint[old + i] = query;
    }

  }

  void ProfilerTimersDX::init( unsigned int timers, ID3D11Device* device, ID3D11DeviceContext* devcontext )
  {
    m_device = device;
    m_context = devcontext;
    TimerEnsureSize(timers);
  }

  void ProfilerTimersDX::deinit()
  {
    if (m_queries.empty()) return;

    for (size_t i=0; i < m_queries.size(); i++){
      m_queries[i]->Release();
      m_queriesDisjoint[i]->Release();
    }
    m_queries.clear();
    m_queriesDisjoint.clear();
  }

  void ProfilerTimersDX::TimerFlush()
  {
    m_context->Flush();
  }

}

