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


#include "base_dx12.hpp"
#include "error_dx12.hpp"
#include <dxgi1_5.h>

#include <algorithm>

namespace nvdx12 {

//////////////////////////////////////////////////////////////////////////


D3D12_RESOURCE_BARRIER transitionBarrier(_In_ ID3D12Resource*  pResource,
                                         D3D12_RESOURCE_STATES stateBefore,
                                         D3D12_RESOURCE_STATES stateAfter,
                                         UINT subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/,
                                         D3D12_RESOURCE_BARRIER_FLAGS flags /*= D3D12_RESOURCE_BARRIER_FLAG_NONE*/)
{
  D3D12_RESOURCE_BARRIER result;
  ZeroMemory(&result, sizeof(result));
  result.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  result.Flags                  = flags;
  result.Transition.pResource   = pResource;
  result.Transition.StateBefore = stateBefore;
  result.Transition.StateAfter  = stateAfter;
  result.Transition.Subresource = subresource;
  return result;
}

//////////////////////////////////////////////////////////////////////////

ID3D12Device* DeviceUtils::createDevice(IDXGIFactory5* factory)
{
  HRESULT        hr              = 0;
  IDXGIAdapter1* hardwareAdapter = nullptr;

  // Look for an actual GPU. This sample does not support WARP (software)
  // devices.
  for(UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex)
  {
    DXGI_ADAPTER_DESC1 desc;
    hardwareAdapter->GetDesc1(&desc);

    if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
    {
      // Don't select the Basic Render Driver adapter.
      continue;
    }

    // Check to see if the adapter supports Direct3D 12, but don't create the
    // actual device yet.
    if(SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
    {
      break;
    }
  }
  if(hardwareAdapter == nullptr)
  {
    return nullptr;
  }


  DXGI_ADAPTER_DESC1 adapterDesc;
  hardwareAdapter->GetDesc1(&adapterDesc);
  printf("Running on DXGI Adapter %S\n", adapterDesc.Description);

  // Create the DX12 device on the selected GPU
  hr = D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
  if(FAILED(hr))
  {
    return nullptr;
  }
  hardwareAdapter->Release();
  return m_device;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC DeviceUtils::createDefaultPipelineDesc(D3D12_INPUT_ELEMENT_DESC* inputDescs,
                                                                          UINT                      inputCount,
                                                                          ID3D12RootSignature*      rootSignature,
                                                                          void*                     vertexShaderPointer,
                                                                          size_t                    vertexShaderSize,
                                                                          void*                     pixelShaderPointer,
                                                                          size_t                    pixelShaderSize)
{

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputDescs, inputCount};
  psoDesc.pRootSignature                     = rootSignature;

  psoDesc.VS = {vertexShaderPointer, vertexShaderSize};
  psoDesc.PS = {pixelShaderPointer, pixelShaderSize};

  psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
  psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
  psoDesc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
  psoDesc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  psoDesc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  psoDesc.RasterizerState.DepthClipEnable       = TRUE;
  psoDesc.RasterizerState.MultisampleEnable     = FALSE;
  psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
  psoDesc.RasterizerState.ForcedSampleCount     = 0;
  psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  psoDesc.BlendState.AlphaToCoverageEnable  = FALSE;
  psoDesc.BlendState.IndependentBlendEnable = FALSE;

  const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
      FALSE,
      FALSE,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_LOGIC_OP_NOOP,
      D3D12_COLOR_WRITE_ENABLE_ALL,
  };
  for(UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

  psoDesc.DepthStencilState.DepthEnable   = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask                      = UINT_MAX;
  psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                = 1;
  psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.SampleDesc.Count                = 1;

  return psoDesc;
}

void DeviceUtils::addDepthStencilTestToPipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
                                                bool                                enableDepth /*= true*/,
                                                bool                                enableStencil /*= false*/,
                                                DXGI_FORMAT                         format /*= DXGI_FORMAT_D32_FLOAT*/)
{
  D3D12_DEPTH_STENCIL_DESC depthStencilState;
  depthStencilState.DepthEnable                     = enableDepth ? TRUE : FALSE;
  depthStencilState.DepthWriteMask                  = D3D12_DEPTH_WRITE_MASK_ALL;
  depthStencilState.DepthFunc                       = D3D12_COMPARISON_FUNC_LESS;
  depthStencilState.StencilEnable                   = enableStencil ? TRUE : FALSE;
  depthStencilState.StencilReadMask                 = D3D12_DEFAULT_STENCIL_READ_MASK;
  depthStencilState.StencilWriteMask                = D3D12_DEFAULT_STENCIL_WRITE_MASK;
  const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
                                                       D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
  depthStencilState.FrontFace                       = defaultStencilOp;
  depthStencilState.BackFace                        = defaultStencilOp;

  psoDesc.DepthStencilState = depthStencilState;
  psoDesc.DSVFormat         = format;
}

ID3D12Resource* DeviceUtils::createBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
{
  D3D12_RESOURCE_DESC bufDesc = {};
  bufDesc.Alignment           = 0;
  bufDesc.DepthOrArraySize    = 1;
  bufDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufDesc.Flags               = flags;
  bufDesc.Format              = DXGI_FORMAT_UNKNOWN;
  bufDesc.Height              = 1;
  bufDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  bufDesc.MipLevels           = 1;
  bufDesc.SampleDesc.Count    = 1;
  bufDesc.SampleDesc.Quality  = 0;
  bufDesc.Width               = size;

  ID3D12Resource* pBuffer;
  HRESULT         hr = 0;
  hr                 = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer));
  if(FAILED(hr))
  {
    return nullptr;
  }
  return pBuffer;
}

ID3D12RootSignature* DeviceUtils::createRootSignature(D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc)
{
  HRESULT hr = 0;

  ID3DBlob*            serializedRootSignature;
  ID3DBlob*            error;
  ID3D12RootSignature* rootSignature;
  hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &error);
  if(FAILED(hr))
  {
    fprintf(stderr, "Could not serialize root signature: %s\n", (LPCSTR)(error->GetBufferPointer()));
    error->Release();
    return nullptr;
  }
  hr = m_device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
                                     serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

  serializedRootSignature->Release();
  if(FAILED(hr))
  {
    fprintf(stderr, "Could not create root signature\n");
    return nullptr;
  }

  return rootSignature;
}

}  // namespace nvdx12
