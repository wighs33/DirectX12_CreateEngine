#pragma once

#include "FrameConstantBuffer.h"
#include "Material.h"
#include "SceneLighting.h"

#include <DirectXMath.h>
#include <d3d12.h>

#include <cstddef>

class Camera;
class D3D12CommandContext;
class ShadowCamera;

// 물체, 재질, Frame의 값을 Shader와 같은 배치로 바꿔 상수 버퍼에 쓰고 연결합니다.
class SceneConstants final
{
public:
    SceneConstants() = default;
    ~SceneConstants() = default;

    SceneConstants(const SceneConstants&) = delete;
    SceneConstants& operator=(const SceneConstants&) = delete;

    bool EnsureCapacity(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        std::size_t objectCount,
        std::size_t materialCount);

    void WriteFrame(
        UINT frameIndex,
        const SceneLighting& lighting,
        const Camera& camera,
        const ShadowCamera& shadowCamera,
        const DirectX::XMMATRIX& viewProjectionMatrix);
    void WriteObject(
        UINT frameIndex,
        std::size_t objectIndex,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMMATRIX& viewProjectionMatrix);
    void WriteMaterial(UINT frameIndex, const Material& material);

    // 미리 쓴 칸의 주소를 Root CBV에 연결합니다.
    void BindFrame(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex) const;
    void BindObject(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        std::size_t objectIndex) const;
    void BindMaterial(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex,
        const Material& material) const;

private:
    // Shader의 b0과 같은 배치로 보낼 값입니다.
    struct ObjectConstants
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 worldInverseTranspose;
    };

    // Shader의 b1과 같은 배치로 보낼 값입니다.
    struct FrameConstants
    {
        LightingConstants lighting;
        DirectX::XMFLOAT3 cameraPosition;
        // 화면의 세로 / 가로 비율입니다.
        float screenHeightOverWidth;
        DirectX::XMFLOAT4X4 viewProjection;
        DirectX::XMFLOAT4X4 lightViewProjection;
        // 그림자를 그릴 때 빛의 반대쪽과 표면 안쪽으로 밀어내는 길이입니다.
        float shadowDepthBias;
        float shadowNormalBias;
        // 그림자 맵 한 칸의 UV 크기입니다.
        float shadowTexelUv;
        float padding;
    };

    // Object와 재질마다 한 칸, 빛과 카메라는 Frame마다 한 칸을 사용합니다.
    FrameConstantBuffer objectConstants_{ static_cast<UINT>(sizeof(ObjectConstants)) };
    FrameConstantBuffer materialConstants_{ static_cast<UINT>(sizeof(MaterialConstants)) };
    FrameConstantBuffer frameConstants_{ static_cast<UINT>(sizeof(FrameConstants)) };
};
