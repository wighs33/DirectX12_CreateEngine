#include "DepthBuffer.h"

#include "D3D12Config.h"

#include <cstdio>

bool DepthBuffer::Initialize(
    ID3D12Device* device,
    UINT width,
    UINT height,
    D3D12_RESOURCE_STATES initialState)
{
    if (width == 0 || height == 0)
    {
        std::fprintf(stderr, "Depth Buffer 생성에 사용할 크기가 0입니다.\n");
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
        IID_PPV_ARGS(&descriptorHeap_));

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
    // 깊이로 쓴 값을 Shader가 읽을 수도 있도록 크기만 32비트로 정하고 해석은 View에 맡깁니다.
    resourceDescription.Format = DXGI_FORMAT_R32_TYPELESS;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 일반 Depth 범위에서 가장 먼 값인 1.0으로 빠르게 Clear할 수 있도록 최적 Clear 값을 제공합니다.
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = D3D12Config::DepthFormat;
    clearValue.DepthStencil.Depth = FarthestDepth;
    clearValue.DepthStencil.Stencil = 0;

    const HRESULT resourceResult = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        initialState,
        &clearValue,
        IID_PPV_ARGS(&resource_));

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
    dsvDescription.Format = D3D12Config::DepthFormat;
    dsvDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescription.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescription.Texture2D.MipSlice = 0;

    dsvHandle_ = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(
        resource_.Get(),
        &dsvDescription,
        dsvHandle_);

    return true;
}

void DepthBuffer::Clear(ID3D12GraphicsCommandList* commandList) const
{
    // [12단계] 이전 Frame의 깊이가 남지 않도록 Depth Buffer 전체를 가장 먼 값 1.0으로 지웁니다.
    commandList->ClearDepthStencilView(
        dsvHandle_,
        D3D12_CLEAR_FLAG_DEPTH,
        FarthestDepth,
        0,
        0,
        nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSV() const
{
    return dsvHandle_;
}

ID3D12Resource* DepthBuffer::GetResource() const
{
    return resource_.Get();
}
