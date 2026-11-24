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
    return rtvHeap_.GetCpuHandle(frameIndex);
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
    // Flip Model SwapChain은 sRGB 형식이나 여러 표본으로 만들 수 없어 장면은 SceneTarget에 따로 그립니다.
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

    if (!rtvHeap_.Initialize(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            FrameCount,
            false))
    {
        return false;
    }

    // Heap의 각 칸에 같은 번호의 BackBuffer를 참조하는 RTV를 생성합니다.
    for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex)
    {
        D3D12_RENDER_TARGET_VIEW_DESC description{};
        description.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        description.Format = D3D12Config::BackBufferFormat;
        device->CreateRenderTargetView(
            backBuffers_[frameIndex].Get(),
            &description,
            GetRTV(frameIndex));
    }

    return true;
}
