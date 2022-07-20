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


#ifndef NV_DX12_BASE_INCLUDED
#define NV_DX12_BASE_INCLUDED

#include <assert.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <platform.h>
#include <vector>

/// \todo Detect swap chain size
#define D3D12_SWAP_CHAIN_SIZE 3

namespace nvdx12 {


//////////////////////////////////////////////////////////////////////////
/**
  \class nvdx12::DeviceUtils
  Utility class for simple creation of pipeline states, root signatures,
  and buffers.
*/

/**
  \fn nvdx12::transitionBarrier
  Short-hand function to create a transition barrier
*/

// Specifies a heap used for uploading. This heap type has CPU access optimized for uploading to the GPU.
static const D3D12_HEAP_PROPERTIES uploadHeapProps = {D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                      D3D12_MEMORY_POOL_UNKNOWN, 0, 0};

// Specifies the default heap. This heap type experiences the most bandwidth for the GPU, but cannot provide CPU access.
static const D3D12_HEAP_PROPERTIES defaultHeapProps = {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                       D3D12_MEMORY_POOL_UNKNOWN, 0, 0};


//////////////////////////////////////////////////////////////////////////

D3D12_RESOURCE_BARRIER transitionBarrier(_In_ ID3D12Resource*  pResource,
                                         D3D12_RESOURCE_STATES stateBefore,
                                         D3D12_RESOURCE_STATES stateAfter,
                                         UINT                  subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                         D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

struct DeviceUtils
{
  DeviceUtils()
      : m_device(nullptr)
  {
  }
  DeviceUtils(ID3D12Device* device)
      : m_device(device)
  {
  }

  ID3D12Device* createDevice(IDXGIFactory5* factory);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC createDefaultPipelineDesc(D3D12_INPUT_ELEMENT_DESC* inputDescs,
                                                               UINT                      inputCount,
                                                               ID3D12RootSignature*      rootSignature,
                                                               void*                     vertexShaderPointer,
                                                               size_t                    vertexShaderSize,
                                                               void*                     pixelShaderPointer,
                                                               size_t                    pixelShaderSize);

  void addDepthStencilTestToPipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
                                     bool                                enableDepth   = true,
                                     bool                                enableStencil = false,
                                     DXGI_FORMAT                         format        = DXGI_FORMAT_D32_FLOAT);

  ID3D12Resource* createBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

  ID3D12RootSignature* createRootSignature(D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc);

  ID3D12Device* m_device;
};

}  // namespace nvdx12

#endif
