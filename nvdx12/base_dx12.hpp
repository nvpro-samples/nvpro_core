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

#ifndef NV_DX12_BASE_INCLUDED
#define NV_DX12_BASE_INCLUDED

#include <assert.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <platform.h>
#include <vector>

// TODO: Detect swap chain size
#define D3D12_SWAP_CHAIN_SIZE 3

namespace nvdx12 {

  // Specifies a heap used for uploading. This heap type has CPU access optimized for uploading to the GPU.
  static const D3D12_HEAP_PROPERTIES uploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

  // Specifies the default heap. This heap type experiences the most bandwidth for the GPU, but cannot provide CPU access.
  static const D3D12_HEAP_PROPERTIES defaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

 

  class Submission {
  private:
    ID3D12CommandQueue *m_queue = nullptr;
    std::vector<ID3D12GraphicsCommandList *> m_commands;
    ID3D12Fence* m_fence = nullptr;
    unsigned int m_fenceValue = 1;
    HANDLE m_fenceEvent;
  public:

    void init(ID3D12Fence* fence);
    void deinit();

    uint32_t getCommandBufferCount() const;

    // can change queue if nothing is pending
    void setQueue(ID3D12CommandQueue *queue);

    void enqueue(uint32_t num, ID3D12GraphicsCommandList **cmdlists);
    void enqueue(ID3D12GraphicsCommandList *cmdlist);
    void enqueueAt(uint32_t pos, ID3D12GraphicsCommandList *cmdbuffer);

    // submits the work and resets internal state
    void execute();
    void reset();

    void flush();
    std::vector<ID3D12GraphicsCommandList *> &getCommandBuffers();
  };

  //////////////////////////////////////////////////////////////////////////

  D3D12_RESOURCE_BARRIER transitionBarrier(
      _In_ ID3D12Resource *pResource, D3D12_RESOURCE_STATES stateBefore,
      D3D12_RESOURCE_STATES stateAfter,
      UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
      D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

  struct DeviceUtils {
    DeviceUtils() : m_device(nullptr) {}
    DeviceUtils(ID3D12Device *device) : m_device(device) {}

    ID3D12Device *createDevice(IDXGIFactory5 *factory);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC createDefaultPipelineDesc(
        D3D12_INPUT_ELEMENT_DESC *inputDescs, UINT inputCount,
        ID3D12RootSignature *rootSignature, void *vertexShaderPointer,
        size_t vertexShaderSize, void *pixelShaderPointer,
        size_t pixelShaderSize);

    void addDepthStencilTestToPipeline(
        D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc, bool enableDepth = true,
        bool enableStencil = false, DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT);

    ID3D12Resource *createBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags,
                                 D3D12_RESOURCE_STATES initState,
                                 const D3D12_HEAP_PROPERTIES &heapProps);

    ID3D12RootSignature* createRootSignature(D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc);

    ID3D12Device *m_device;
  };



  class RingCmdPool {
  public:
    void init(ID3D12Device *device, ID3D12CommandQueue *commandQueue);
    void deinit();
    void reset();

    // call once per cycle prior creating command buffers
    // resets old pools etc.
    void setCycle(uint32_t cycleIndex);

    ID3D12GraphicsCommandList *createCommandList(
        ID3D12PipelineState *pipelineState,
        D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    void execute();

  private:
    uint32_t m_currentFrame = 0;
    ID3D12Device *m_device = nullptr;
    ID3D12CommandQueue *m_commandQueue; // s[D3D12_SWAP_CHAIN_SIZE];
    ID3D12CommandAllocator *m_commandAllocators[D3D12_SWAP_CHAIN_SIZE];
    Submission m_cycles[D3D12_SWAP_CHAIN_SIZE];
  };

} // namespace nvdx12

#endif
