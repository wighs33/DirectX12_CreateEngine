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
    if (!CreateDepthBuffer(device, width, height))
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

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetDSV() const
{
    return dsvHandle_;
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
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

    // 두 BackBuffer의 RTV Descriptor를 보관할 Heap을 생성합니다.
    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDescription.NumDescriptors = FrameCount;
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
        rtvHandles_[frameIndex] = rtvHandle;
        device->CreateRenderTargetView(
            backBuffers_[frameIndex].Get(),
            nullptr,
            rtvHandles_[frameIndex]);

        rtvHandle.ptr += rtvDescriptorSize_;
    }

    return true;
}

bool D3D12SwapChain::CreateDepthBuffer(ID3D12Device* device, UINT width, UINT height)
{
    if (width == 0 || height == 0)
    {
        std::fprintf(stderr, "Depth Buffer 생성에 사용할 화면 크기가 0입니다.\n");
        return false;
    }

    // 하나의 Depth Stencil View를 보관할 CPU 전용 DSV Descriptor Heap을 생성합니다.
    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDescription.NumDescriptors = 1;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDescription.NodeMask = 0;

    const HRESULT heapResult = device->CreateDescriptorHeap(
        &heapDescription,
        IID_PPV_ARGS(&dsvDescriptorHeap_));

    if (FAILED(heapResult))
    {
        std::fprintf(
            stderr,
            "DSV용 ID3D12Device::CreateDescriptorHeap 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(heapResult));
        return false;
    }

    // Depth Buffer는 GPU가 읽고 쓰는 32비트 부동소수점 Texture2D이므로 Default Heap에 생성합니다.
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Alignment = 0;
    resourceDescription.Width = width;
    resourceDescription.Height = height;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_D32_FLOAT;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 일반 Depth 범위에서 가장 먼 값인 1.0으로 빠르게 Clear할 수 있도록 최적 Clear 값을 제공합니다.
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    const HRESULT resourceResult = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthBuffer_));

    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "Depth Buffer용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    // Texture2D Depth Resource를 Output Merger가 사용할 수 있도록 DSV Descriptor로 연결합니다.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescription{};
    dsvDescription.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescription.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescription.Texture2D.MipSlice = 0;

    dsvHandle_ = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(
        depthBuffer_.Get(),
        &dsvDescription,
        dsvHandle_);

    return true;
}
