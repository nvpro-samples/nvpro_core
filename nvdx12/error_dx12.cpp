/* Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include "error_dx12.hpp"

#include <nvh/nvprint.hpp>

namespace nvdx12 {

const char* getResultString(HRESULT hr)
{
  const char* resultString = "unknown";

#define STR(a)                                                                                                         \
  case a:                                                                                                              \
    resultString = #a;                                                                                                 \
    break;

  switch(hr)
  {
    STR(S_OK);
    STR(E_FAIL);
    STR(E_INVALIDARG);
    STR(E_OUTOFMEMORY);
    STR(DXGI_ERROR_INVALID_CALL);
    STR(DXGI_ERROR_WAS_STILL_DRAWING);
    STR(D3D12_ERROR_ADAPTER_NOT_FOUND);
    STR(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
  }
#undef STR
  return resultString;
}

bool checkResult(HRESULT hr, const char* message)
{
  if(SUCCEEDED(hr))
  {
    return false;
  }

  if(message)
  {
    LOGE("HRESULT %l - %s - %s\n", hr, getResultString(hr), message);
  }
  else
  {
    LOGE("HRESULT %l - %s\n", hr, getResultString(hr));
  }
  assert(!"Critical DX12 Error");
  return true;
}

bool checkResult(HRESULT hr, const char* file, int line)
{
  if(SUCCEEDED(hr))
  {
    return false;
  }

  LOGE("%s(%l): DX12 Error : %s\n", file, line, getResultString(hr));
  assert(!"Critical DX12 Error");

  return true;
}

}  // namespace nvdx12
