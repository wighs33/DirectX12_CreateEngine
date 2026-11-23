#include "SceneConstants.h"

#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12RootSignature.h"
#include "ShadowCamera.h"
#include "ShadowMap.h"

namespace
{
    // 그림자 맵 한 칸보다 작은 요철은 담기지 않으므로, 자기 표면에 그림자가 얼룩지지 않도록 칸의 몇 배를 밀어냅니다.
    // 시야의 반지름이 4일 때 Pandora Arena의 그림자와 같은 길이(약 3mm, 14mm)가 됩니다.
    constexpr float ShadowDepthBias = 0.75f;
    constexpr float ShadowNormalBias = 3.7f;
}

bool SceneConstants::EnsureCapacity(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    std::size_t objectCount,
    std::size_t materialCount)
{
    return objectConstants_.EnsureCapacity(device, commandContext, objectCount) &&
        materialConstants_.EnsureCapacity(device, commandContext, materialCount) &&
        frameConstants_.EnsureCapacity(device, commandContext, 1);
}

void SceneConstants::WriteFrame(
    UINT frameIndex,
    const SceneLighting& lighting,
    const Camera& camera,
    const ShadowCamera& shadowCamera,
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    FrameConstants constants{};
    constants.lighting = lighting.CreateConstants();
    constants.cameraPosition = camera.GetPosition();
    constants.screenHeightOverWidth = 1.0f / camera.GetAspectRatio();
    DirectX::XMStoreFloat4x4(
        &constants.viewProjection,
        DirectX::XMMatrixTranspose(viewProjectionMatrix));
    DirectX::XMStoreFloat4x4(
        &constants.lightViewProjection,
        DirectX::XMMatrixTranspose(shadowCamera.GetViewProjectionMatrix()));
    constants.shadowDepthBias = ShadowDepthBias * shadowCamera.GetTexelSize();
    constants.shadowNormalBias = ShadowNormalBias * shadowCamera.GetTexelSize();
    constants.shadowTexelUv = 1.0f / static_cast<float>(ShadowMap::Resolution);
    frameConstants_.Write(frameIndex, 0, &constants);
}

void SceneConstants::WriteObject(
    UINT frameIndex,
    std::size_t objectIndex,
    const DirectX::XMMATRIX& worldMatrix,
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    const DirectX::XMMATRIX worldViewProjectionMatrix = DirectX::XMMatrixMultiply(
        worldMatrix,
        viewProjectionMatrix);
    const DirectX::XMMATRIX worldInverseTransposeMatrix = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixInverse(nullptr, worldMatrix));

    ObjectConstants constants{};
    // DirectXMath의 행 우선 Matrix를 HLSL의 기본 열 우선 메모리 배치와 맞추기 위해 전치합니다.
    DirectX::XMStoreFloat4x4(
        &constants.worldViewProjection,
        DirectX::XMMatrixTranspose(worldViewProjectionMatrix));
    DirectX::XMStoreFloat4x4(
        &constants.world,
        DirectX::XMMatrixTranspose(worldMatrix));
    DirectX::XMStoreFloat4x4(
        &constants.worldInverseTranspose,
        DirectX::XMMatrixTranspose(worldInverseTransposeMatrix));
    objectConstants_.Write(frameIndex, objectIndex, &constants);
}

void SceneConstants::WriteMaterial(UINT frameIndex, const Material& material)
{
    const MaterialConstants constants = material.CreateConstants();
    materialConstants_.Write(
        frameIndex,
        material.GetConstantSlot(),
        &constants);
}

void SceneConstants::BindFrame(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex) const
{
    commandList->SetGraphicsRootConstantBufferView(
        static_cast<UINT>(RootParameter::Frame),
        frameConstants_.GetGpuAddress(frameIndex, 0));
}

void SceneConstants::BindObject(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    std::size_t objectIndex) const
{
    // [14단계] 같은 b0 Root CBV에 현재 Object 전용 Constant Buffer 주소를 연결합니다.
    commandList->SetGraphicsRootConstantBufferView(
        static_cast<UINT>(RootParameter::Object),
        objectConstants_.GetGpuAddress(frameIndex, objectIndex));
}

void SceneConstants::BindMaterial(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const Material& material) const
{
    // b2에 재질의 그림체 값을 연결합니다.
    commandList->SetGraphicsRootConstantBufferView(
        static_cast<UINT>(RootParameter::Material),
        materialConstants_.GetGpuAddress(
            frameIndex,
            material.GetConstantSlot()));
}
