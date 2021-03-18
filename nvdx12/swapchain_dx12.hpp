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

#ifndef NV_DX12_SWAPCHAIN_INCLUDED
#define NV_DX12_SWAPCHAIN_INCLUDED

#include <d3d12.h>
#include <dxgi1_5.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <nvdx12/base_dx12.hpp>

namespace nvdx12 {
  class SwapChain {
  private:
    DXGI_FORMAT m_format;
    IDXGIFactory5 *m_factory;
    ID3D12Device *m_device;

    uint32_t m_currentImage;
    uint32_t m_currentSemaphore;
    HWND m_hWnd;
    ID3D12CommandQueue *m_commandQueue;
    IDXGISwapChain3 *m_swapChain = nullptr;
    ID3D12DescriptorHeap *m_renderTargetViewHeap;
    UINT m_rtvDescriptorSize;
    ID3D12Fence *m_fence;
    HANDLE m_fenceEvent;
    ID3D12Resource *m_renderTargets[D3D12_SWAP_CHAIN_SIZE];
    UINT64 m_fenceValues[D3D12_SWAP_CHAIN_SIZE];

    UINT m_width = ~0u;
    UINT m_height = ~0u;
    UINT m_syncInterval = 0;

  public:
    void init(HWND hWnd, IDXGIFactory5 *factory, ID3D12Device *device,
              ID3D12CommandQueue *commandQueue);
    void update(int width, int height);
    void deinit();
    // Flags are defined by the DXGI_PRESENT_* constants
    void present(UINT flags = 0);

    // Present the current image after n blank syncs (0 for no sync)
    void setSyncInterval(UINT value) { m_syncInterval = value; }
    UINT getSyncInterval() const { return m_syncInterval; }

    IDXGISwapChain3* getSwapChain() const { return m_swapChain; }
    ID3D12Resource* getActiveImage();

    void setRenderTarget(
        ID3D12GraphicsCommandList *commandList, UINT numRenderTargets = 1,
        const D3D12_CPU_DESCRIPTOR_HANDLE *depthStencilDesc = nullptr);
    void clearRenderTarget(ID3D12GraphicsCommandList *commandList,
                           const float rgba[4], UINT numRects = 0,
                           D3D12_RECT *rects = nullptr);

    void moveToNextFrame();
    // Wait for pending GPU work to complete.
    void waitForGpu();
    // Be sure to wait for GPU first
    void releaseRenderTargets();
    void createRenderTargetViews();

    UINT getWidth() const { return m_width; }
    UINT getHeight() const { return m_height; }

    D3D12_RESOURCE_BARRIER getPresentToRenderTargetBarrier() const;
    D3D12_RESOURCE_BARRIER getRenderTargetToPresentBarrier() const;
    uint32_t getImageCount() const;
    uint32_t getCurrentImageIndex() const;
  };
} // namespace nvdx12

#endif
