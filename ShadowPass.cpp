#include "ShadowPass.h"

#include "GameObject.h"
#include "Model.h"
#include "RenderPassContext.h"
#include "RenderStatistics.h"
#include "SceneConstants.h"
#include "ShadowMap.h"

#include <memory>

namespace
{
    // 빛과 나란한 면은 그림자 맵 한 칸 안에서도 깊이가 크게 바뀌므로 그만큼 뒤로 밉니다.
    constexpr float SlopeScaledDepthBias = 5.0f;
}

bool ShadowPass::Initialize(
    ID3D12Device* device,
    const D3D12RootSignature& rootSignature)
{
    // 색을 쓰지 않으므로 Pixel Shader가 없고 깊이만 남습니다.
    PipelineSettings settings;
    settings.vertexShader = "VSShadow";
    settings.pixelShader = nullptr;
    settings.cullMode = D3D12_CULL_MODE_BACK;
    settings.slopeScaledDepthBias = SlopeScaledDepthBias;
    return pipeline_.Initialize(device, rootSignature, settings);
}

void ShadowPass::Draw(const RenderPassContext& context) const
{
    ID3D12GraphicsCommandList* const commandList = context.commandList;

    // [7~11단계] 화면 대신 그림자 맵을 그릴 대상으로 연결하고 지웁니다.
    context.shadowMap.BeginWrite(commandList);

    // [12~16단계] 깊이만 쓰는 PSO로 빛에 보이는 조각을 모두 그립니다. 재질과 Texture는 필요 없습니다.
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
            part->mesh.Draw(commandList);
            context.statistics.AddDraw(part->mesh.GetTriangleCount());
        }
    }

    // 다음 패스가 Texture로 읽을 수 있게 합니다.
    context.shadowMap.EndWrite(commandList);
}
