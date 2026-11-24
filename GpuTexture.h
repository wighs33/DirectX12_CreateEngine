#pragma once

#include <d3d12.h>
#include <wrl.h>

// 만들 Texture의 크기와 쓰임입니다.
struct GpuTextureDescription
{
    UINT width = 0;
    UINT height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    // 한 픽셀에 둘 표본의 수입니다.
    UINT sampleCount = 1;
    // 그릴 대상이나 깊이로 쓸 때 필요한 표시입니다.
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    // 이 값으로 지우면 GPU가 더 빠르게 지울 수 있습니다.
    const D3D12_CLEAR_VALUE* clearValue = nullptr;
};

// GPU만 읽고 쓰는 메모리에 2D Texture를 만듭니다.
class GpuTexture final
{
public:
    GpuTexture() = delete;

    static bool Create(
        ID3D12Device* device,
        const GpuTextureDescription& description,
        Microsoft::WRL::ComPtr<ID3D12Resource>& texture);
};
