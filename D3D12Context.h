#pragma once

#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12Config.h"
#include "D3D12DeviceResources.h"
#include "D3D12Pipeline.h"
#include "D3D12SwapChain.h"
#include "Mesh.h"
#include "Scene.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

// 주제별 DirectX 12 객체를 연결하고 전체 초기화와 한 Frame의 렌더링 순서를 조정합니다.
class D3D12Context final
{
public:
    D3D12Context() = default;
    ~D3D12Context() = default;

    D3D12Context(const D3D12Context&) = delete;
    D3D12Context& operator=(const D3D12Context&) = delete;

    // HWND에 연결할 DirectX 12 객체를 순서대로 생성합니다.
    bool Initialize(HWND window, UINT width, UINT height);

    // Keyboard/Mouse 입력과 DeltaTime을 이용해 CPU 측 Camera 상태를 갱신합니다.
    void Update(float deltaTime);

    // 현재 BackBuffer를 지정한 색으로 지우고 화면에 표시하는 한 프레임을 실행합니다.
    bool RenderFrame();

private:
    struct ConstantBufferData
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
    };

    static constexpr UINT ConstantBufferAlignment =
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    static constexpr UINT AlignedConstantBufferSize =
        (static_cast<UINT>(sizeof(ConstantBufferData)) + ConstantBufferAlignment - 1) &
        ~(ConstantBufferAlignment - 1);

    bool CreateCubeMesh();
    bool CreateTexture();
    bool CreateConstantBuffer(UINT width, UINT height);
    void UpdateConstantBuffers(UINT frameIndex);

    // Camera는 Scene을 바라보는 View Matrix만 제공하며 GameObject와 별도로 유지합니다.
    Camera camera_;

    D3D12DeviceResources deviceResources_;
    D3D12SwapChain swapChain_;
    D3D12Pipeline pipeline_;

    // Mesh를 Scene보다 먼저 생성하고 나중에 파괴하여 GameObject의 비소유 Mesh 포인터를 안전하게 유지합니다.
    Mesh cubeMesh_;
    Scene scene_;

    // 모든 Cube가 공유하는 Texture Resource와 이를 Shader에 공개하는 SRV Heap입니다.
    Microsoft::WRL::ComPtr<ID3D12Resource> texture_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    // Upload Heap을 한 번 Map한 뒤 Frame별 WVP Matrix를 갱신할 때 계속 사용할 CPU 주소입니다.
    void* constantBufferMappedData_ = nullptr;
    UINT clientWidth_ = 0;
    UINT clientHeight_ = 0;
    HWND window_ = nullptr;

    // 마지막에 선언하여 소멸 시 다른 GPU Resource보다 먼저 대기와 동기화 정리를 수행합니다.
    D3D12CommandContext commandContext_;
};
