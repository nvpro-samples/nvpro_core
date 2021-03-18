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

#include "swapchain_dx12.hpp"
#include <assert.h>

#define CHECK_DX12_RESULT() assert(SUCCEEDED(hr))

namespace nvdx12 {
  void SwapChain::init(HWND hWnd, IDXGIFactory5 *factory, ID3D12Device *device,
                       ID3D12CommandQueue *commandQueue) {

    m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_factory = factory;
    m_device = device;
    m_hWnd = hWnd;
    m_commandQueue = commandQueue;
    for (int i = 0; i < D3D12_SWAP_CHAIN_SIZE; i++) {
      m_fenceValues[i] = 0;
    }
  }

  void SwapChain::update(int width, int height) {
    HRESULT hr = 0;

    if (width == m_width && height == m_height)
      return;
    m_width  = width;
    m_height = height;
    if (m_swapChain != nullptr) {
      waitForGpu();
      bool recreateRenderTargetViews = m_renderTargets[0] != nullptr;
      releaseRenderTargets();
      DXGI_SWAP_CHAIN_DESC desc;
      hr = m_swapChain->GetDesc(&desc);
      CHECK_DX12_RESULT();
      hr = m_swapChain->ResizeBuffers(D3D12_SWAP_CHAIN_SIZE, width, height, desc.BufferDesc.Format, desc.Flags);
      CHECK_DX12_RESULT();
      if(recreateRenderTargetViews) {
        createRenderTargetViews();
      }
      return;
    }
    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width                 = width;
    swapChainDesc.Height                = height;
    swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count      = 1;
    swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount           = D3D12_SWAP_CHAIN_SIZE;
    swapChainDesc.Scaling               = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags                 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    IDXGISwapChain1 *swapChain1;

    hr = m_factory->CreateSwapChainForHwnd(
        m_commandQueue, // Swap chain needs the queue so that it can force a
                        // flush on it.
        m_hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1);

    hr = swapChain1->QueryInterface(&m_swapChain);
    CHECK_DX12_RESULT();

    swapChain1->Release();

    // TODO: does not support full screen transitions yet. Check swap chain code
    // when removing this flag
    hr = m_factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

    m_currentImage = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.

    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = D3D12_SWAP_CHAIN_SIZE;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = m_device->CreateDescriptorHeap(&rtvHeapDesc,
                                        IID_PPV_ARGS(&m_renderTargetViewHeap));

    CHECK_DX12_RESULT();

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    createRenderTargetViews();

    // Create the fence for buffer presentation
    hr = m_device->CreateFence(m_fenceValues[m_currentImage],
                               D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    m_fenceValues[m_currentImage]++;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
      hr = HRESULT_FROM_WIN32(GetLastError());
      CHECK_DX12_RESULT();
    }
  }

  void SwapChain::createRenderTargetViews() {
    HRESULT hr;
    
    // Create frame resources.
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each frame.
    for(UINT n = 0; n < D3D12_SWAP_CHAIN_SIZE; n++)
    {
      hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
      CHECK_DX12_RESULT();

      m_device->CreateRenderTargetView(m_renderTargets[n], nullptr, rtvHandle);
      rtvHandle.ptr += m_rtvDescriptorSize;
    }
  }

  void SwapChain::deinit() {
    m_renderTargetViewHeap->Release();

    releaseRenderTargets();
    m_fence->Release();
    m_swapChain->Release();
  }

  ID3D12Resource * SwapChain::getActiveImage()
  {
    UINT backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    return m_renderTargets[backBufferIndex];
  }

  void SwapChain::setRenderTarget(
      ID3D12GraphicsCommandList *commandList, UINT numRenderTargets /*=1*/,
      const D3D12_CPU_DESCRIPTOR_HANDLE *depthStencilDesc /*=nullptr*/) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    rtvHandle.ptr =
        m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart().ptr +
        m_currentImage * m_rtvDescriptorSize;

    commandList->OMSetRenderTargets(numRenderTargets, &rtvHandle, FALSE,
                                    depthStencilDesc);
  }

  void SwapChain::clearRenderTarget(ID3D12GraphicsCommandList *commandList,
                                    const float rgba[4], UINT numRects /*= 0*/,
                                    D3D12_RECT *rects /*= nullptr*/) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    rtvHandle.ptr =
        m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart().ptr +
        m_currentImage * m_rtvDescriptorSize;
    commandList->ClearRenderTargetView(rtvHandle, rgba, numRects, rects);
  }

  // Prepare to render the next frame.
  void SwapChain::moveToNextFrame() {
    // Schedule a Signal command in the queue.
    HRESULT hr = 0;
    const UINT64 currentFenceValue = m_fenceValues[m_currentImage];
    hr = m_commandQueue->Signal(m_fence, currentFenceValue);

    // Update the frame index.
    m_currentImage = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is
    // ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_currentImage]) {
      hr = m_fence->SetEventOnCompletion(m_fenceValues[m_currentImage],
                                         m_fenceEvent);
      WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_currentImage] = currentFenceValue + 1;
  }

  // Wait for pending GPU work to complete.
  void SwapChain::waitForGpu() {
    // Schedule a Signal command in the queue.
    HRESULT hr = 0;
    hr = m_commandQueue->Signal(m_fence, m_fenceValues[m_currentImage]);

    // Wait until the fence has been processed.
    hr = m_fence->SetEventOnCompletion(m_fenceValues[m_currentImage],
                                       m_fenceEvent);
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_currentImage]++;
  }

  void SwapChain::releaseRenderTargets() {
    for(UINT n = 0; n < D3D12_SWAP_CHAIN_SIZE; ++n) {
      if(m_renderTargets[n] != nullptr) {
        m_renderTargets[n]->Release();
        m_renderTargets[n] = nullptr;
      }
    }
  }

  D3D12_RESOURCE_BARRIER SwapChain::getPresentToRenderTargetBarrier() const {
    return nvdx12::transitionBarrier(
        m_renderTargets[m_currentImage], D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
  }

  D3D12_RESOURCE_BARRIER SwapChain::getRenderTargetToPresentBarrier() const {
    return nvdx12::transitionBarrier(
        m_renderTargets[m_currentImage], D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
  }

  void SwapChain::present(UINT flags) {
    m_swapChain->Present(m_syncInterval, flags);
  }

  uint32_t SwapChain::getImageCount() const { return D3D12_SWAP_CHAIN_SIZE; }

  uint32_t SwapChain::getCurrentImageIndex() const { return m_currentImage; }

} // namespace nvdx12
