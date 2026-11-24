#include "SceneRenderer.h"

#include "Camera.h"
#include "GameObject.h"
#include "Model.h"
#include "RenderPassContext.h"
#include "RenderStatistics.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "TextureHeap.h"

#include <algorithm>
#include <iterator>

bool SceneRenderer::Initialize(ID3D12Device* device, TextureHeap& textureHeap)
{
    return rootSignature_.Initialize(device) &&
        shadowMap_.Initialize(device, textureHeap) &&
        shadowPass_.Initialize(device, rootSignature_) &&
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

void SceneRenderer::BeginFrame(
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

    shadowCamera_.Update(scene, camera);
    CollectObjects(scene, viewProjectionMatrix, statistics);
    WriteConstants(frameIndex, scene, camera, viewProjectionMatrix);
    BindCommonState(commandList, frameIndex, resourceManager);
}

void SceneRenderer::RenderShadow(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const ResourceManager& resourceManager,
    RenderStatistics& statistics) const
{
    // 화면 밖의 물체도 그림자를 드리우므로 빛의 시야에 들어온 물체를 그립니다.
    const RenderPassContext context
    {
        commandList,
        frameIndex,
        shadowCuller_.GetVisibleObjects(),
        constants_,
        resourceManager.GetTextureHeap(),
        shadowMap_,
        statistics
    };
    shadowPass_.Draw(context);
}

void SceneRenderer::RenderScene(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const ResourceManager& resourceManager,
    RenderStatistics& statistics) const
{
    const RenderPassContext context
    {
        commandList,
        frameIndex,
        culler_.GetVisibleObjects(),
        constants_,
        resourceManager.GetTextureHeap(),
        shadowMap_,
        statistics
    };
    // 물체를 먼저 그려 두면 안쪽에 가려진 외곽선은 깊이 비교에서 지워집니다.
    toonPass_.Draw(context);
    outlinePass_.Draw(context);
}

void SceneRenderer::CollectObjects(
    const Scene& scene,
    const DirectX::XMMATRIX& viewProjectionMatrix,
    RenderStatistics& statistics)
{
    culler_.Collect(scene, viewProjectionMatrix);
    shadowCuller_.Collect(scene, shadowCamera_.GetViewProjectionMatrix());

    const std::vector<VisibleObject>& visibleObjects = culler_.GetVisibleObjects();
    const std::vector<VisibleObject>& shadowObjects = shadowCuller_.GetVisibleObjects();
    statistics.SetVisibility(
        visibleObjects.size(),
        culler_.GetCulledObjectCount());

    // 두 목록 모두 Scene의 순서대로이므로 번호를 견주어 합칩니다.
    drawnObjects_.clear();
    std::set_union(
        visibleObjects.begin(),
        visibleObjects.end(),
        shadowObjects.begin(),
        shadowObjects.end(),
        std::back_inserter(drawnObjects_),
        [](const VisibleObject& left, const VisibleObject& right)
        {
            return left.objectIndex < right.objectIndex;
        });
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
        shadowCamera_,
        viewProjectionMatrix);

    for (const VisibleObject& object : drawnObjects_)
    {
        constants_.WriteObject(
            frameIndex,
            object.objectIndex,
            object.gameObject->GetTransform().GetWorldMatrix(),
            viewProjectionMatrix);
    }

    // 그림자 패스는 재질을 쓰지 않으므로 재질은 화면에 보이는 물체만 씁니다.
    for (const VisibleObject& object : culler_.GetVisibleObjects())
    {
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
    // [5단계] b0~b2 Constant Buffer와 t0~t1 Texture를 사용할 Root Signature를 연결합니다.
    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    // 모든 Texture의 SRV가 들어 있는 Heap을 연결하고 어느 칸을 쓸지는 조각마다 정합니다.
    ID3D12DescriptorHeap* const descriptorHeaps[] =
    {
        resourceManager.GetTextureHeap().GetHeap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    constants_.BindFrame(commandList, frameIndex);

    // [6단계] Input Assembler가 Vertex를 세 개씩 묶는 Triangle List를 사용하도록 설정합니다.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
