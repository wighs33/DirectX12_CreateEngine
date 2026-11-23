#include "D3D12SwapChain.h"

#include <cstdio>

using Microsoft::WRL::ComPtr;

bool D3D12SwapChain::Initialize(
    IDXGIFactory6* factory,
    ID3D12Device* device,
    ID3D12CommandQueue* commandQueue,
    HWND window,
    UINT width,
    UINT height)
{
    if (factory == nullptr || device == nullptr || commandQueue == nullptr || window == nullptr)
    {
        std::fprintf(stderr, "D3D12SwapChain::Initialize에 유효하지 않은 초기화 객체가 전달되었습니다.\n");
        return false;
    }

    // Clip Space 결과를 전체 Client 영역으로 변환할 Viewport를 저장합니다.
    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    // 전체 Client 영역을 통과시키는 Scissor Rect를 저장합니다.
    scissorRect_.left = 0;
    scissorRect_.top = 0;
    scissorRect_.right = static_cast<LONG>(width);
    scissorRect_.bottom = static_cast<LONG>(height);

    if (!CreateSwapChain(factory, commandQueue, window, width, height))
        return false;
    if (!CreateRenderTargets(device))
        return false;

    return true;
}

UINT D3D12SwapChain::GetCurrentFrameIndex()
{
    currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    return currentBackBufferIndex_;
}

ID3D12Resource* D3D12SwapChain::GetBackBuffer(UINT frameIndex) const
{
    return backBuffers_[frameIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetRTV(UINT frameIndex) const
{
    return rtvHandles_[frameIndex];
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetEditorRTV(UINT frameIndex) const
{
    return editorRtvHandles_[frameIndex];
}

const D3D12_VIEWPORT& D3D12SwapChain::GetViewport() const
{
    return viewport_;
}

const D3D12_RECT& D3D12SwapChain::GetScissorRect() const
{
    return scissorRect_;
}

bool D3D12SwapChain::Present()
{
    const HRESULT result = swapChain_->Present(1, 0);
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "IDXGISwapChain::Present 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    return true;
}

bool D3D12SwapChain::CreateSwapChain(
    IDXGIFactory6* factory,
    ID3D12CommandQueue* commandQueue,
    HWND window,
    UINT width,
    UINT height)
{
    // [8단계] HWND와 Command Queue를 연결해 Double Buffer SwapChain을 생성합니다.
    // SwapChain : 화면 표시용 이미지 Buffer들을 관리하고, 완성된 이미지를 화면으로 보내는 역할 (Front Buffer, Back Buffer)
    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = width;
    description.Height = height;
    // Flip Model SwapChain은 sRGB 형식으로 만들 수 없으므로 RTV에서만 sRGB로 해석합니다.
    description.Format = D3D12Config::BackBufferFormat;
    description.Stereo = FALSE;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = FrameCount;
    description.Scaling = DXGI_SCALING_STRETCH;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    description.Flags = 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    const HRESULT creationResult = factory->CreateSwapChainForHwnd(
        commandQueue,
        window,
        &description,
        nullptr, // 전체 화면 설명을 생략하면 Windowed 모드로 생성됩니다.
        nullptr,
        &swapChain1);

    if (FAILED(creationResult))
    {
        std::fprintf(
            stderr,
            "IDXGIFactory::CreateSwapChainForHwnd 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(creationResult));
        return false;
    }

    // 현재 BackBuffer 번호를 얻을 수 있는 IDXGISwapChain3 인터페이스로 변환합니다.
    const HRESULT interfaceResult = swapChain1.As(&swapChain_);
    if (FAILED(interfaceResult))
    {
        std::fprintf(
            stderr,
            "IDXGISwapChain3 인터페이스 변환에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(interfaceResult));
        return false;
    }

    currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12SwapChain::CreateRenderTargets(ID3D12Device* device)
{
    // [9단계] SwapChain의 두 BackBuffer Resource를 가져옵니다.
    for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex)
    {
        const HRESULT result = swapChain_->GetBuffer(
            frameIndex,
            IID_PPV_ARGS(&backBuffers_[frameIndex]));

        if (FAILED(result))
        {
            std::fprintf(
                stderr,
                "IDXGISwapChain::GetBuffer(%u) 호출에 실패했습니다. HRESULT: 0x%08X\n",
                frameIndex,
                static_cast<unsigned int>(result));
            return false;
        }
    }

    // 두 BackBuffer마다 Scene용과 ImGui용 RTV Descriptor를 보관할 Heap을 생성합니다.
    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDescription.NumDescriptors = FrameCount * 2;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDescription.NodeMask = 0;

    const HRESULT heapResult = device->CreateDescriptorHeap(
        &heapDescription,
        IID_PPV_ARGS(&rtvDescriptorHeap_));

    if (FAILED(heapResult))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateDescriptorHeap 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(heapResult));
        return false;
    }

    rtvDescriptorSize_ =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Heap의 각 슬롯에 같은 인덱스의 BackBuffer를 참조하는 RTV를 생성합니다.
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDescription{};
        rtvDescription.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        rtvDescription.Format = D3D12Config::SceneTargetFormat;
        rtvHandles_[frameIndex] = rtvHandle;
        device->CreateRenderTargetView(
            backBuffers_[frameIndex].Get(),
            &rtvDescription,
            rtvHandles_[frameIndex]);
        rtvHandle.ptr += rtvDescriptorSize_;

        rtvDescription.Format = D3D12Config::BackBufferFormat;
        editorRtvHandles_[frameIndex] = rtvHandle;
        device->CreateRenderTargetView(
            backBuffers_[frameIndex].Get(),
            &rtvDescription,
            editorRtvHandles_[frameIndex]);
        rtvHandle.ptr += rtvDescriptorSize_;
    }

    return true;
}
