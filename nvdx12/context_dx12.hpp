/* Copyright (c) 2016-2018, NVIDIA CORPORATION. All rights reserved.
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

#ifndef NV_DX12_CONTEXT_INCLUDED
#define NV_DX12_CONTEXT_INCLUDED

#include <d3d12.h>
#include <dxgi1_5.h>
#include <vector>

namespace nvdx12 {

struct ContextCreateInfo
{
  UINT compatibleAdapterIndex = 0;

  // Information printed at Context::init time
  bool verboseCompatibleAdapters = false;
};

class Context {
public:
  Context(Context const&) = delete;
  Context& operator=(Context const&) = delete;

  Context() = default;

  IDXGIFactory5 *m_factory;
  ID3D12Device *m_device;
  ID3D12CommandQueue *m_commandQueue;

  bool init(const ContextCreateInfo& info);
  void deinit();

  std::vector<IDXGIAdapter1*> getCompatibleAdapters(const ContextCreateInfo& info);
};

} // namespace nvdx12

#endif
