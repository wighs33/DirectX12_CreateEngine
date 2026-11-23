#include "Model.h"

#include <DirectXMath.h>

#include <limits>
#include <utility>

bool Model::Initialize(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const std::string& id,
    const ModelData& modelData,
    const std::vector<const Texture*>& materialTextures)
{
    std::vector<std::unique_ptr<ModelPart>> parts;
    for (const ModelPartData& partData : modelData.parts)
    {
        auto part = std::make_unique<ModelPart>();
        part->name = partData.name;
        if (partData.materialIndex >= materialTextures.size())
        {
            return false;
        }
        part->texture = materialTextures[partData.materialIndex];

        // 32bit Index와 D3D12 Buffer View가 다룰 수 있는 개수인지는 Mesh가 다시 확인합니다.
        if (partData.vertices.size() > (std::numeric_limits<UINT>::max)() ||
            partData.indices.size() > (std::numeric_limits<UINT>::max)() ||
            !part->mesh.Initialize(
                device,
                commandContext,
                partData.vertices.data(),
                static_cast<UINT>(partData.vertices.size()),
                partData.indices.data(),
                static_cast<UINT>(partData.indices.size())))
        {
            return false;
        }

        parts.push_back(std::move(part));
    }
    if (parts.empty())
    {
        return false;
    }

    // 모든 조각을 감싸는 상자를 구해 컬링과 선택에 함께 사용합니다.
    DirectX::XMVECTOR minimum =
        DirectX::XMLoadFloat3(&parts.front()->mesh.GetLocalBounds().minimum);
    DirectX::XMVECTOR maximum =
        DirectX::XMLoadFloat3(&parts.front()->mesh.GetLocalBounds().maximum);
    for (const std::unique_ptr<ModelPart>& part : parts)
    {
        const AABB& bounds = part->mesh.GetLocalBounds();
        minimum = DirectX::XMVectorMin(
            minimum,
            DirectX::XMLoadFloat3(&bounds.minimum));
        maximum = DirectX::XMVectorMax(
            maximum,
            DirectX::XMLoadFloat3(&bounds.maximum));
    }

    DirectX::XMStoreFloat3(&localBounds_.minimum, minimum);
    DirectX::XMStoreFloat3(&localBounds_.maximum, maximum);
    parts_ = std::move(parts);
    id_ = id;
    return true;
}

const std::string& Model::GetId() const
{
    return id_;
}

const std::vector<std::unique_ptr<ModelPart>>& Model::GetParts() const
{
    return parts_;
}

const AABB& Model::GetLocalBounds() const
{
    return localBounds_;
}
