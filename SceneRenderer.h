#pragma once

#include "D3D12RootSignature.h"
#include "OutlinePass.h"
#include "SceneConstants.h"
#include "SceneVisibility.h"
#include "ShadowCamera.h"
#include "ShadowMap.h"
#include "ShadowPass.h"
#include "ToonPass.h"

#include <DirectXMath.h>
#include <d3d12.h>

class Camera;
class D3D12CommandContext;
class RenderStatistics;
class ResourceManager;
class Scene;
class TextureHeap;

// 보이는 물체를 고르고 값을 쓴 뒤 패스를 정해진 순서로 그립니다.
class SceneRenderer final
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    // Root Signature와 그림자 맵, 패스마다 쓸 PSO를 만듭니다.
    bool Initialize(ID3D12Device* device, TextureHeap& textureHeap);

    // 명령을 기록하기 전에 이번 Frame의 물체와 재질 수만큼 상수 버퍼의 칸을 확보합니다.
    bool Prepare(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const Scene& scene,
        const ResourceManager& resourceManager);

    // 이번 Frame에 그릴 물체를 고르고 값을 쓴 뒤 모든 패스가 함께 쓰는 것을 연결합니다.
    void BeginFrame(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const Scene& scene,
        const Camera& camera,
        const ResourceManager& resourceManager,
        RenderStatistics& statistics);

    // 그릴 대상을 그림자 맵으로 바꾸므로 화면을 연결하기 전에 부릅니다.
    void RenderShadow(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const ResourceManager& resourceManager,
        RenderStatistics& statistics) const;

    // 이미 연결된 Render Target에 장면을 그립니다.
    void RenderScene(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const ResourceManager& resourceManager,
        RenderStatistics& statistics) const;

private:
    // 고른 물체와 그 재질의 값을 상수 버퍼에 씁니다.
    void WriteConstants(
        UINT frameIndex,
        const Scene& scene,
        const Camera& camera,
        const DirectX::XMMATRIX& viewProjectionMatrix);
    void BindCommonState(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const ResourceManager& resourceManager) const;

    D3D12RootSignature rootSignature_;
    SceneVisibility visibility_;
    SceneConstants constants_;

    ShadowCamera shadowCamera_;
    ShadowMap shadowMap_;
    ShadowPass shadowPass_;
    ToonPass toonPass_;
    OutlinePass outlinePass_;
};
