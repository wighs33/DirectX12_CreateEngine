#include "SceneRenderer.h"

#include "BoundingBox.h"
#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12Pipeline.h"
#include "GameObject.h"
#include "Model.h"
#include "RenderStatistics.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "Texture.h"
#include "TextureHeap.h"

#include <memory>
#include <vector>

namespace
{
    // Root Signature에서 정한 Parameter 순서입니다.
    constexpr UINT ObjectRootParameter = 0;
    constexpr UINT TextureRootParameter = 1;
    constexpr UINT FrameRootParameter = 2;
    constexpr UINT MaterialRootParameter = 3;
}

bool SceneRenderer::Prepare(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const Scene& scene,
    const ResourceManager& resourceManager)
{
    return objectConstants_.EnsureCapacity(
            device,
            commandContext,
            scene.GetGameObjectCount()) &&
        materialConstants_.EnsureCapacity(
            device,
            commandContext,
            resourceManager.GetMaterialCount()) &&
        frameConstants_.EnsureCapacity(device, commandContext, 1);
}

void SceneRenderer::Render(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const D3D12Pipeline& pipeline,
    const Scene& scene,
    const Camera& camera,
    const ResourceManager& resourceManager,
    RenderStatistics& statistics)
{
    const DirectX::XMMATRIX viewProjectionMatrix = DirectX::XMMatrixMultiply(
        camera.GetViewMatrix(),
        camera.GetProjectionMatrix());

    BindFrame(
        commandList,
        frameIndex,
        pipeline,
        scene.GetLighting(),
        camera,
        viewProjectionMatrix,
        resourceManager);
    CollectVisibleObjects(frameIndex, scene, viewProjectionMatrix, statistics);

    // 물체를 먼저 그려 두면 안쪽에 가려진 외곽선은 깊이 비교에서 지워집니다.
    DrawToonPass(commandList, frameIndex, pipeline, resourceManager, statistics);
    DrawOutlinePass(commandList, frameIndex, pipeline, statistics);
}

void SceneRenderer::BindFrame(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const D3D12Pipeline& pipeline,
    const SceneLighting& lighting,
    const Camera& camera,
    const DirectX::XMMATRIX& viewProjectionMatrix,
    const ResourceManager& resourceManager)
{
    // [11단계] b0~b2 Constant Buffer와 t0 Texture를 사용할 Root Signature를 연결합니다.
    commandList->SetGraphicsRootSignature(pipeline.GetRootSignature());

    // 모든 Texture의 SRV가 들어 있는 Heap을 연결하고 어느 칸을 쓸지는 조각마다 정합니다.
    ID3D12DescriptorHeap* const descriptorHeaps[] =
    {
        resourceManager.GetTextureHeap().GetHeap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    FrameConstants constants{};
    constants.lighting = lighting.CreateConstants();
    constants.cameraPosition = camera.GetPosition();
    constants.screenHeightOverWidth = 1.0f / camera.GetAspectRatio();
    DirectX::XMStoreFloat4x4(
        &constants.viewProjection,
        DirectX::XMMatrixTranspose(viewProjectionMatrix));
    frameConstants_.Write(frameIndex, 0, &constants);
    commandList->SetGraphicsRootConstantBufferView(
        FrameRootParameter,
        frameConstants_.GetGpuAddress(frameIndex, 0));

    // [12단계] Input Assembler가 Vertex를 세 개씩 묶는 Triangle List를 사용하도록 설정합니다.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SceneRenderer::CollectVisibleObjects(
    UINT frameIndex,
    const Scene& scene,
    const DirectX::XMMATRIX& viewProjectionMatrix,
    RenderStatistics& statistics)
{
    // GPU 업로드용 Transpose Matrix가 아닌 CPU 원본 ViewProjection으로 Frame당 한 번 갱신합니다.
    frustum_.Update(viewProjectionMatrix);
    visibleObjects_.clear();

    // Object를 직접 소유하지 않고 Scene이 제공하는 목록을 순회합니다.
    const std::vector<GameObject>& gameObjects = scene.GetGameObjects();
    for (std::size_t objectIndex = 0; objectIndex < gameObjects.size(); ++objectIndex)
    {
        const GameObject& gameObject = gameObjects[objectIndex];
        const Model* const model = gameObject.GetModel();
        if (model == nullptr)
        {
            continue;
        }

        if (!frustum_.Intersects(gameObject.GetWorldBounds()))
        {
            statistics.AddCulledObject();
            continue;
        }

        WriteObjectConstants(
            frameIndex,
            objectIndex,
            gameObject.GetTransform().GetWorldMatrix(),
            viewProjectionMatrix);
        visibleObjects_.push_back({ objectIndex, model });
        statistics.AddRenderedObject();
    }
}

void SceneRenderer::DrawToonPass(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const D3D12Pipeline& pipeline,
    const ResourceManager& resourceManager,
    RenderStatistics& statistics)
{
    // [13단계] 두 단 명암을 그리는 Graphics Pipeline State를 연결합니다.
    commandList->SetPipelineState(pipeline.GetToonPipelineState());

    for (const VisibleObject& object : visibleObjects_)
    {
        BindObject(commandList, frameIndex, object.objectIndex);
        for (const std::unique_ptr<ModelPart>& part : object.model->GetParts())
        {
            const Material& material =
                object.model->GetMaterials()[part->materialIndex];

            // 외곽선도 같은 칸을 읽으므로 재질의 값은 여기서 한 번만 씁니다.
            const MaterialConstants constants = material.CreateConstants();
            materialConstants_.Write(
                frameIndex,
                material.GetConstantSlot(),
                &constants);
            BindMaterial(commandList, frameIndex, material);

            // t0 Table이 재질의 Texture 칸을 가리키게 합니다.
            commandList->SetGraphicsRootDescriptorTable(
                TextureRootParameter,
                resourceManager.GetTextureHeap().GetGpuHandle(
                    material.GetTexture().GetDescriptorSlot()));
            DrawPart(commandList, *part, statistics);
        }
    }
}

void SceneRenderer::DrawOutlinePass(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const D3D12Pipeline& pipeline,
    RenderStatistics& statistics)
{
    // [13~17단계] 외곽선 PSO로 바꿔 같은 조각을 한 번 더 그립니다.
    commandList->SetPipelineState(pipeline.GetOutlinePipelineState());

    for (const VisibleObject& object : visibleObjects_)
    {
        BindObject(commandList, frameIndex, object.objectIndex);
        for (const std::unique_ptr<ModelPart>& part : object.model->GetParts())
        {
            const Material& material =
                object.model->GetMaterials()[part->materialIndex];
            if (!material.HasOutline())
            {
                continue;
            }

            BindMaterial(commandList, frameIndex, material);
            DrawPart(commandList, *part, statistics);
        }
    }
}

void SceneRenderer::WriteObjectConstants(
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

void SceneRenderer::BindObject(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    std::size_t objectIndex) const
{
    // [14단계] 같은 b0 Root CBV에 현재 Object 전용 Constant Buffer 주소를 연결합니다.
    commandList->SetGraphicsRootConstantBufferView(
        ObjectRootParameter,
        objectConstants_.GetGpuAddress(frameIndex, objectIndex));
}

void SceneRenderer::BindMaterial(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const Material& material) const
{
    // b2에 재질의 그림체 값을 연결합니다.
    commandList->SetGraphicsRootConstantBufferView(
        MaterialRootParameter,
        materialConstants_.GetGpuAddress(
            frameIndex,
            material.GetConstantSlot()));
}

void SceneRenderer::DrawPart(
    ID3D12GraphicsCommandList* commandList,
    const ModelPart& part,
    RenderStatistics& statistics)
{
    // [15단계] 현재 조각의 Vertex Buffer View를 Input Slot 0에 연결합니다.
    const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView =
        part.mesh.GetVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // [16단계] 같은 조각의 Index Buffer View를 연결합니다.
    const D3D12_INDEX_BUFFER_VIEW& indexBufferView =
        part.mesh.GetIndexBufferView();
    commandList->IASetIndexBuffer(&indexBufferView);

    // [17단계] 모델의 조각 하나당 한 번씩 Indexed Draw 명령을 기록합니다.
    commandList->DrawIndexedInstanced(part.mesh.GetIndexCount(), 1, 0, 0, 0);
    statistics.AddDraw(part.mesh.GetIndexCount() / 3);
}
