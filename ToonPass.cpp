#include "ToonPass.h"

#include "D3D12RootSignature.h"
#include "GameObject.h"
#include "Model.h"
#include "RenderPassContext.h"
#include "RenderStatistics.h"
#include "SceneConstants.h"
#include "Texture.h"
#include "TextureHeap.h"

#include <memory>

bool ToonPass::Initialize(
    ID3D12Device* device,
    const D3D12RootSignature& rootSignature)
{
    PipelineSettings settings;
    settings.vertexShader = "VSMain";
    settings.pixelShader = "PSMain";
    settings.cullMode = D3D12_CULL_MODE_BACK;
    return pipeline_.Initialize(device, rootSignature, settings);
}

void ToonPass::Draw(const RenderPassContext& context) const
{
    ID3D12GraphicsCommandList* const commandList = context.commandList;

    // [13단계] 두 단 명암을 그리는 Graphics Pipeline State를 연결합니다.
    commandList->SetPipelineState(pipeline_.Get());

    for (const VisibleObject& object : context.visibleObjects)
    {
        context.constants.BindObject(
            commandList,
            context.frameIndex,
            object.objectIndex);

        const Model& model = *object.gameObject->GetModel();
        for (const std::unique_ptr<ModelPart>& part : model.GetParts())
        {
            const Material& material =
                model.GetMaterials()[part->materialIndex];
            context.constants.BindMaterial(
                commandList,
                context.frameIndex,
                material);

            // t0 Table이 재질의 Texture 칸을 가리키게 합니다.
            commandList->SetGraphicsRootDescriptorTable(
                static_cast<UINT>(RootParameter::Texture),
                context.textureHeap.GetGpuHandle(
                    material.GetTexture().GetDescriptorSlot()));

            part->mesh.Draw(commandList);
            context.statistics.AddDraw(part->mesh.GetTriangleCount());
        }
    }
}
