#pragma once

#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12Config.h"
#include "D3D12DeviceResources.h"
#include "D3D12Pipeline.h"
#include "D3D12SwapChain.h"
#include "FrameConstantBuffer.h"
#include "Frustum.h"
#include "ImGuiLayer.h"
#include "Model.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SceneEditor.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <cstddef>
#include <cstdint>

struct RenderingStats
{
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    std::uint32_t sceneObjectCount = 0;
    std::uint32_t renderedObjectCount = 0;
    std::uint32_t culledObjectCount = 0;
    std::uint32_t drawCallCount = 0;
    std::uint32_t triangleCount = 0;
};

// 주제별 DirectX 12 객체를 연결하고 전체 초기화와 한 Frame의 렌더링 순서를 조정합니다.
class D3D12Context final
{
public:
    D3D12Context() = default;
    ~D3D12Context();

    D3D12Context(const D3D12Context&) = delete;
    D3D12Context& operator=(const D3D12Context&) = delete;

    // HWND에 연결할 DirectX 12 객체를 순서대로 생성합니다.
    bool Initialize(HWND window, UINT width, UINT height);

    // Editor Frame을 시작하고 입력과 편집 UI를 처리합니다.
    void Update(float deltaTime);

    // 현재 BackBuffer를 지정한 색으로 지우고 화면에 표시하는 한 프레임을 실행합니다.
    bool RenderFrame();

private:
    struct ConstantBufferData
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 worldInverseTranspose;
    };

    struct LightingConstantBufferData
    {
        DirectX::XMFLOAT4 lightDirection;
        DirectX::XMFLOAT4 lightColor;
        DirectX::XMFLOAT4 ambientColor;
        DirectX::XMFLOAT4 cameraPosition;
    };

    bool CreatePrimitiveModels();
    void UpdateLightingConstantBuffer(UINT frameIndex);
    void BindMaterial(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const ModelMaterial& material);
    void UpdateObjectConstantBuffer(
        UINT frameIndex,
        std::size_t objectIndex,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMMATRIX& viewProjectionMatrix);
    void UpdateRenderingStatistics(float deltaTime);
    void UpdateWindowTitle() const;

    // Camera는 Scene을 바라보는 View Matrix만 제공하며 GameObject와 별도로 유지합니다.
    Camera camera_;
    Frustum frustum_;

    D3D12DeviceResources deviceResources_;
    D3D12SwapChain swapChain_;
    D3D12Pipeline pipeline_;

    // ResourceManager는 공유 Texture와 Model의 수명, 그리고 Texture를 연결할 SRV Heap을 소유합니다.
    // Scene보다 먼저 생성하고 나중에 파괴하여 GameObject의 비소유 Model 포인터를 안전하게 유지합니다.
    ResourceManager resourceManager_;
    Scene scene_;

    // Scene을 편집하는 UI와 그것을 그리는 ImGui Backend입니다.
    ImGuiLayer imguiLayer_;
    SceneEditor sceneEditor_;

    // Object와 재질마다 한 칸, 조명은 Frame마다 한 칸을 사용합니다.
    FrameConstantBuffer objectConstants_{ static_cast<UINT>(sizeof(ConstantBufferData)) };
    FrameConstantBuffer materialConstants_{ static_cast<UINT>(sizeof(MaterialConstants)) };
    FrameConstantBuffer lightingConstants_{ static_cast<UINT>(sizeof(LightingConstantBufferData)) };
    RenderingStats renderingStats_;
    float statisticsElapsedTime_ = 0.0f;
    std::uint32_t statisticsFrameCount_ = 0;
    HWND window_ = nullptr;

    // 마지막에 선언하여 소멸 시 다른 GPU Resource보다 먼저 대기와 동기화 정리를 수행합니다.
    D3D12CommandContext commandContext_;
};
