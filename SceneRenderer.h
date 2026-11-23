#pragma once

#include "FrameConstantBuffer.h"
#include "Frustum.h"
#include "Material.h"
#include "SceneLighting.h"

#include <DirectXMath.h>
#include <d3d12.h>

#include <cstddef>

class Camera;
class D3D12CommandContext;
class D3D12Pipeline;
class GameObject;
class RenderStatistics;
class ResourceManager;
class Scene;
struct ModelPart;

// 장면의 물체를 골라 Draw 명령을 기록합니다.
// 물체, 재질, 조명 값을 GPU에 넘길 상수 버퍼를 소유합니다.
class SceneRenderer final
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    // 명령을 기록하기 전에 이번 Frame의 물체와 재질 수만큼 상수 버퍼의 칸을 확보합니다.
    bool Prepare(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const Scene& scene,
        const ResourceManager& resourceManager);

    // 이미 연결된 Render Target에 장면을 그리고 그린 양을 statistics에 적습니다.
    void Render(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const D3D12Pipeline& pipeline,
        const Scene& scene,
        const Camera& camera,
        const ResourceManager& resourceManager,
        RenderStatistics& statistics);

private:
    // Shader의 b0과 같은 배치로 보낼 값입니다.
    struct ObjectConstants
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 worldInverseTranspose;
    };

    void BindFrame(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const D3D12Pipeline& pipeline,
        const SceneLighting& lighting,
        const Camera& camera,
        const ResourceManager& resourceManager);
    void BindObject(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        std::size_t objectIndex,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMMATRIX& viewProjectionMatrix);
    void BindMaterial(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const Material& material,
        const ResourceManager& resourceManager);
    static void DrawPart(
        ID3D12GraphicsCommandList* commandList,
        const ModelPart& part,
        RenderStatistics& statistics);

    Frustum frustum_;

    // Object와 재질마다 한 칸, 조명은 Frame마다 한 칸을 사용합니다.
    FrameConstantBuffer objectConstants_{ static_cast<UINT>(sizeof(ObjectConstants)) };
    FrameConstantBuffer materialConstants_{ static_cast<UINT>(sizeof(MaterialConstants)) };
    FrameConstantBuffer lightingConstants_{ static_cast<UINT>(sizeof(LightingConstants)) };
};
