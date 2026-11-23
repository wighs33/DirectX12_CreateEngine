#include "OutlinePass.h"

#include "GameObject.h"
#include "Model.h"
#include "RenderPassContext.h"
#include "RenderStatistics.h"
#include "SceneConstants.h"

#include <memory>

bool OutlinePass::Initialize(
    ID3D12Device* device,
    const D3D12RootSignature& rootSignature)
{
    // 앞면을 제거하고 남은 뒷면을 VSOutline이 바깥쪽으로 밀어냅니다.
    PipelineSettings settings;
    settings.vertexShader = "VSOutline";
    settings.pixelShader = "PSOutline";
    settings.cullMode = D3D12_CULL_MODE_FRONT;
    return pipeline_.Initialize(device, rootSignature, settings);
}

void OutlinePass::Draw(const RenderPassContext& context) const
{
    ID3D12GraphicsCommandList* const commandList = context.commandList;

    // [13~17단계] 외곽선 PSO로 바꿔 같은 조각을 한 번 더 그립니다.
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
            if (!material.HasOutline())
            {
                continue;
            }

            context.constants.BindMaterial(
                commandList,
                context.frameIndex,
                material);
            part->mesh.Draw(commandList);
            context.statistics.AddDraw(part->mesh.GetTriangleCount());
        }
    }
}
