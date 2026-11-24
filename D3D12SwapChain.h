#pragma once

#include "D3D12Config.h"
#include "DescriptorHeap.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <array>

// HWND에 연결된 SwapChain과 BackBuffer, 그것을 가리키는 RTV를 함께 관리합니다.
class D3D12SwapChain final
{
public:
    static constexpr UINT FrameCount = D3D12Config::FrameCount;

    D3D12SwapChain() = default;
    ~D3D12SwapChain() = default;

    D3D12SwapChain(const D3D12SwapChain&) = delete;
    D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;

    bool Initialize(
        IDXGIFactory6* factory,
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        HWND window,
        UINT width,
        UINT height);

    UINT GetCurrentFrameIndex();
    ID3D12Resource* GetBackBuffer(UINT frameIndex) const;
    // 색을 변환하지 않고 쓰는 ImGui용 RTV입니다.
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(UINT frameIndex) const;
    bool Present();

private:
    bool CreateSwapChain(
        IDXGIFactory6* factory,
        ID3D12CommandQueue* commandQueue,
        HWND window,
        UINT width,
        UINT height);
    bool CreateRenderTargets(ID3D12Device* device);

    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_;
    UINT currentBackBufferIndex_ = 0;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> backBuffers_;

    // BackBuffer마다 한 칸을 사용합니다.
    DescriptorHeap rtvHeap_;
};
