#include "DepthBuffer.h"

#include "D3D12Config.h"
#include "GpuTexture.h"

bool DepthBuffer::Initialize(
    ID3D12Device* device,
    UINT width,
    UINT height,
    UINT sampleCount,
    D3D12_RESOURCE_STATES initialState)
{
    // 일반 Depth 범위에서 가장 먼 값인 1.0으로 빠르게 Clear할 수 있도록 최적 Clear 값을 제공합니다.
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = D3D12Config::DepthFormat;
    clearValue.DepthStencil.Depth = FarthestDepth;
    clearValue.DepthStencil.Stencil = 0;

    GpuTextureDescription description;
    description.width = width;
    description.height = height;
    // 깊이로 쓴 값을 Shader가 읽을 수도 있도록 크기만 32비트로 정하고 해석은 View에 맡깁니다.
    description.format = DXGI_FORMAT_R32_TYPELESS;
    description.sampleCount = sampleCount;
    description.flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    description.initialState = initialState;
    description.clearValue = &clearValue;
    if (!GpuTexture::Create(device, description, resource_) ||
        !descriptorHeap_.Initialize(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            1,
            false))
    {
        return false;
    }

    // Texture2D Depth Resource를 Output Merger가 사용할 수 있도록 DSV Descriptor로 연결합니다.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescription{};
    dsvDescription.Format = D3D12Config::DepthFormat;
    dsvDescription.ViewDimension = sampleCount > 1
        ? D3D12_DSV_DIMENSION_TEXTURE2DMS
        : D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescription.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescription.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(
        resource_.Get(),
        &dsvDescription,
        GetDSV());
    return true;
}

void DepthBuffer::Clear(ID3D12GraphicsCommandList* commandList) const
{
    // [11단계] 이전 Frame의 깊이가 남지 않도록 Depth Buffer 전체를 가장 먼 값 1.0으로 지웁니다.
    commandList->ClearDepthStencilView(
        GetDSV(),
        D3D12_CLEAR_FLAG_DEPTH,
        FarthestDepth,
        0,
        0,
        nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSV() const
{
    return descriptorHeap_.GetCpuHandle(0);
}

ID3D12Resource* DepthBuffer::GetResource() const
{
    return resource_.Get();
}
