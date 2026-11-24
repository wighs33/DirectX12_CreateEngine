#pragma once

#include "DepthBuffer.h"

#include <d3d12.h>
#include <wrl.h>

// 한 픽셀에 표본을 여러 개 두고 장면을 그린 뒤, 평균을 내어 BackBuffer로 옮깁니다.
class SceneTarget final
{
public:
    SceneTarget() = default;
    ~SceneTarget() = default;

    SceneTarget(const SceneTarget&) = delete;
    SceneTarget& operator=(const SceneTarget&) = delete;

    bool Initialize(ID3D12Device* device, UINT width, UINT height);

    // 색과 깊이를 그릴 대상으로 연결하고 이전 Frame의 것을 지웁니다.
    void Begin(ID3D12GraphicsCommandList* commandList) const;

    // 표본의 평균을 BackBuffer에 씁니다. BackBuffer는 RESOLVE_DEST 상태여야 합니다.
    void ResolveTo(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* backBuffer) const;

private:
    bool CreateColorBuffer(ID3D12Device* device, UINT width, UINT height);
    bool CreateRenderTargetView(ID3D12Device* device);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> colorBuffer_;
    DepthBuffer depthBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};
};
