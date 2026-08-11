#pragma once

#include "D3D12Config.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <array>

// HWND에 연결된 SwapChain과 화면 출력용 BackBuffer/RTV를 함께 관리합니다.
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
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(UINT frameIndex) const;
    const D3D12_VIEWPORT& GetViewport() const;
    const D3D12_RECT& GetScissorRect() const;
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

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    UINT rtvDescriptorSize_ = 0;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FrameCount> rtvHandles_{};

    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
};
