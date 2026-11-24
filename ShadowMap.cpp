#include "ShadowMap.h"

#include "GpuBarrier.h"
#include "TextureHeap.h"

bool ShadowMap::Initialize(ID3D12Device* device, TextureHeap& textureHeap)
{
    if (!depthBuffer_.Initialize(
            device,
            Resolution,
            Resolution,
            1,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
    {
        return false;
    }

    // 깊이로 쓴 32비트를 Shader는 실수 하나짜리 색으로 읽습니다.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription{};
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescription.Format = DXGI_FORMAT_R32_FLOAT;
    srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Texture2D.MostDetailedMip = 0;
    srvDescription.Texture2D.MipLevels = 1;
    srvDescription.Texture2D.PlaneSlice = 0;
    srvDescription.Texture2D.ResourceMinLODClamp = 0.0f;

    return textureHeap.Add(
        device,
        depthBuffer_.GetResource(),
        srvDescription,
        descriptorSlot_);
}

void ShadowMap::BeginWrite(ID3D12GraphicsCommandList* commandList) const
{
    GpuBarrier::Transition(
        commandList,
        depthBuffer_.GetResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);

    D3D12_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(Resolution);
    viewport.Height = static_cast<float>(Resolution);
    viewport.MinDepth = D3D12_MIN_DEPTH;
    viewport.MaxDepth = D3D12_MAX_DEPTH;
    commandList->RSSetViewports(1, &viewport);

    const D3D12_RECT scissorRect
    {
        0,
        0,
        static_cast<LONG>(Resolution),
        static_cast<LONG>(Resolution)
    };
    commandList->RSSetScissorRects(1, &scissorRect);

    // 색은 그리지 않으므로 RTV 없이 DSV만 연결합니다.
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthBuffer_.GetDSV();
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
    depthBuffer_.Clear(commandList);
}

void ShadowMap::EndWrite(ID3D12GraphicsCommandList* commandList) const
{
    // 깊이를 모두 쓴 뒤에 읽기를 시작하게 합니다.
    GpuBarrier::Transition(
        commandList,
        depthBuffer_.GetResource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

UINT ShadowMap::GetDescriptorSlot() const
{
    return descriptorSlot_;
}
