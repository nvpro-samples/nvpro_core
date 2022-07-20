/*
 * Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2016-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef NV_DX12_CONTEXT_INCLUDED
#define NV_DX12_CONTEXT_INCLUDED

#include <d3d12.h>
#include <dxgi1_5.h>
#include <vector>

namespace nvdx12 {


////////////////////////////////////////////////////////////////////////////////

/**
  \class nvdx12::Context
  Container class for a basic DX12 app, consisting of a DXGI factory, a DX12
  device, and a command queue.
 */

/**
  \struct nvdx12::ContextCreateInfo
  Properties for context initialization.
*/

struct ContextCreateInfo
{
  UINT compatibleAdapterIndex = 0;

  // Information printed at Context::init time
  bool verboseCompatibleAdapters = false;
};

class Context
{
public:
  Context(Context const&) = delete;
  Context& operator=(Context const&) = delete;

  Context() = default;

  IDXGIFactory5*      m_factory;
  ID3D12Device*       m_device;
  ID3D12CommandQueue* m_commandQueue;

  bool init(const ContextCreateInfo& info);
  void deinit();

  std::vector<IDXGIAdapter1*> getCompatibleAdapters(const ContextCreateInfo& info);
};

}  // namespace nvdx12

#endif
