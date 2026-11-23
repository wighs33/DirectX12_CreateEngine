#pragma once

#include "BoundingBox.h"
#include "Mesh.h"
#include "ModelData.h"

#include <d3d12.h>

#include <memory>
#include <string>
#include <vector>

class D3D12CommandContext;
class Texture;

// 모델을 이루는 조각 하나의 GPU Geometry와 재질입니다.
struct ModelPart
{
    std::string name;
    Mesh mesh;
    // Texture의 수명은 ResourceManager가 관리하며 항상 유효한 Texture를 가리킵니다.
    const Texture* texture = nullptr;
};

// 여러 조각을 하나로 묶어 여러 GameObject가 함께 참조하는 모델입니다.
class Model final
{
public:
    Model() = default;
    ~Model() = default;

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // id는 Scene File에 적히는 이름이며 기본 도형은 이름, 파일 모델은 경로입니다.
    // materialTextures는 modelData.materials와 같은 순서의 Texture입니다.
    bool Initialize(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const std::string& id,
        const ModelData& modelData,
        const std::vector<const Texture*>& materialTextures);

    const std::string& GetId() const;
    const std::vector<std::unique_ptr<ModelPart>>& GetParts() const;
    const AABB& GetLocalBounds() const;

private:
    std::string id_;
    std::vector<std::unique_ptr<ModelPart>> parts_;
    AABB localBounds_{};
};
