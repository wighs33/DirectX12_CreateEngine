#include "SceneRenderer.h"

#include "Camera.h"
#include "GameObject.h"
#include "Model.h"
#include "RenderPassContext.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "TextureHeap.h"

bool SceneRenderer::Initialize(ID3D12Device* device)
{
    return rootSignature_.Initialize(device) &&
        toonPass_.Initialize(device, rootSignature_) &&
        outlinePass_.Initialize(device, rootSignature_);
}

bool SceneRenderer::Prepare(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const Scene& scene,
    const ResourceManager& resourceManager)
{
    return constants_.EnsureCapacity(
        device,
        commandContext,
        scene.GetGameObjectCount(),
        resourceManager.GetMaterialCount());
}

void SceneRenderer::Render(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const Scene& scene,
    const Camera& camera,
    const ResourceManager& resourceManager,
    RenderStatistics& statistics)
{
    const DirectX::XMMATRIX viewProjectionMatrix = DirectX::XMMatrixMultiply(
        camera.GetViewMatrix(),
        camera.GetProjectionMatrix());

    culler_.Collect(scene, viewProjectionMatrix, statistics);
    WriteConstants(frameIndex, scene, camera, viewProjectionMatrix);
    BindCommonState(commandList, frameIndex, resourceManager);

    const RenderPassContext context
    {
        commandList,
        frameIndex,
        culler_.GetVisibleObjects(),
        constants_,
        resourceManager.GetTextureHeap(),
        statistics
    };
    // 물체를 먼저 그려 두면 안쪽에 가려진 외곽선은 깊이 비교에서 지워집니다.
    toonPass_.Draw(context);
    outlinePass_.Draw(context);
}

void SceneRenderer::WriteConstants(
    UINT frameIndex,
    const Scene& scene,
    const Camera& camera,
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    constants_.WriteFrame(
        frameIndex,
        scene.GetLighting(),
        camera,
        viewProjectionMatrix);

    for (const VisibleObject& object : culler_.GetVisibleObjects())
    {
        constants_.WriteObject(
            frameIndex,
            object.objectIndex,
            object.gameObject->GetTransform().GetWorldMatrix(),
            viewProjectionMatrix);
        for (const Material& material :
             object.gameObject->GetModel()->GetMaterials())
        {
            constants_.WriteMaterial(frameIndex, material);
        }
    }
}

void SceneRenderer::BindCommonState(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const ResourceManager& resourceManager) const
{
    // [11단계] b0~b2 Constant Buffer와 t0 Texture를 사용할 Root Signature를 연결합니다.
    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    // 모든 Texture의 SRV가 들어 있는 Heap을 연결하고 어느 칸을 쓸지는 조각마다 정합니다.
    ID3D12DescriptorHeap* const descriptorHeaps[] =
    {
        resourceManager.GetTextureHeap().GetHeap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    constants_.BindFrame(commandList, frameIndex);

    // [12단계] Input Assembler가 Vertex를 세 개씩 묶는 Triangle List를 사용하도록 설정합니다.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
