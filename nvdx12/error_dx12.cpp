/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
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
    LOGE("HRESULT %li - %s - %s\n", hr, getResultString(hr), message);
  }
  else
  {
    LOGE("HRESULT %li - %s\n", hr, getResultString(hr));
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

  LOGE("%s(%i): DX12 Error : %s\n", file, line, getResultString(hr));
  assert(!"Critical DX12 Error");

  return true;
}

}  // namespace nvdx12
