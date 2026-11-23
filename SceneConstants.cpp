#include "SceneConstants.h"

#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12RootSignature.h"

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
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    FrameConstants constants{};
    constants.lighting = lighting.CreateConstants();
    constants.cameraPosition = camera.GetPosition();
    constants.screenHeightOverWidth = 1.0f / camera.GetAspectRatio();
    DirectX::XMStoreFloat4x4(
        &constants.viewProjection,
        DirectX::XMMatrixTranspose(viewProjectionMatrix));
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
