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

#include "context_dx12.hpp"
#include "error_dx12.hpp"

#include <nvh/nvprint.hpp>

namespace nvdx12 {

bool Context::init(const ContextCreateInfo& info) {
  UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
  // Enable the debug layer (requires the Graphics Tools "optional feature").
  // This will allow the driver to output errors and track object leaks
  // NOTE: Enabling the debug layer after device creation will invalidate the
  // active device.
  {
    ID3D12Debug *debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
      debugController->EnableDebugLayer();

      // Enable additional debug layers.
      dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
  }
#endif

  if (HR_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)))) {
    return false;
  }

  auto compatibleAdapters = getCompatibleAdapters(info);
  if (compatibleAdapters.empty())
  {
    assert(!"No compatible adapter found");
    return false;
  }

  // Create the DX12 device on the selected GPU
  const HRESULT hr = D3D12CreateDevice(compatibleAdapters[info.compatibleAdapterIndex], D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));

  // Release all the adapters gathered by getCompatibleAdapters above
  for (IDXGIAdapter1* adapter : compatibleAdapters) {
    adapter->Release();
  }

  if (HR_CHECK(hr)) {
    return false;
  }

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  if (HR_CHECK(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_commandQueue)))) {
    return false;
  }

  return true;
}

void Context::deinit() {
  // Release all DX12 objects of the context
  m_commandQueue->Release();
  m_factory->Release();

#ifdef _DEBUG
  // If the debug layer is enabled, write on stdout whether there are any
  // leaked DX12 objects. Since the device is still alive, the report should
  // indicate a nonzero ID3D12Device reference count, but all other references
  // should have a Refcount: 0. The nonzero IntRef indicates the DX12-internal
  // references to the object, which the driver will release upon release of
  // the device.
  HRESULT hr;
  ID3D12DebugDevice *debugDevice = nullptr;
  hr = m_device->QueryInterface(__uuidof(ID3D12DebugDevice),
                                (void **)(&debugDevice));
  if (SUCCEEDED(hr))
  {
    debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
    debugDevice->Release();
  }
#endif

  // Release the device itself
  unsigned long deviceRefs = 0;
  deviceRefs = m_device->Release();
  assert(deviceRefs == 0 &&
          "Some references to the device have not been released properly");
}

std::vector<IDXGIAdapter1*> Context::getCompatibleAdapters(const ContextCreateInfo& info)
{
  if (info.verboseCompatibleAdapters)
  {
    LOGI("____________________\n");
    LOGI("Compatible Adapters :\n");
  }

  IDXGIAdapter1 *adapter = nullptr;
  std::vector<IDXGIAdapter1*> compatibleAdapters;

  // Find the adapters that represents a GPU device and support Direct3D 12.
  for (UINT adapterIndex = 0; SUCCEEDED(m_factory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    // Ignore the sotware "Basic Render Driver" adapter.
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      adapter->Release();
      continue;
    }

    char description[128 * 4] = "";
    WideCharToMultiByte(CP_UTF8, 0, desc.Description, 128, description, sizeof(description), nullptr, nullptr);

    // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
      if (info.verboseCompatibleAdapters) {
        LOGI("%zu: %s\n", compatibleAdapters.size(), description);
      }
      compatibleAdapters.push_back(adapter);
    }
    else {
      if (info.verboseCompatibleAdapters) {
        LOGW("Skipping adapter %s\n", description);
      }
      adapter->Release();
    }
  }

  if (adapter) {
    adapter->Release();
  }

  if (info.verboseCompatibleAdapters)
  {
    LOGI("Compatible adapters devices found : ");
    if (!compatibleAdapters.empty()) {
      LOGI("%zu\n", compatibleAdapters.size());
    }
    else {
      LOGI("OMG... NONE !!\n");
    }
  }

  return compatibleAdapters;
}

} // namespace nvdx12
