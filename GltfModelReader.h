#pragma once

#include "GltfFile.h"
#include "ModelData.h"

#include <DirectXMath.h>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

// glTF의 노드 계층을 따라가며 재질과 조각을 모아 모델 하나로 만듭니다.
// 실패하면 이유를 담은 예외를 던집니다.
class GltfModelReader final
{
public:
    explicit GltfModelReader(const std::filesystem::path& filePath);

    ModelData Read();

private:
    using Json = nlohmann::json;

    static constexpr int MaximumNodeDepth = 256;
    static constexpr std::size_t InvalidIndex =
        (std::numeric_limits<std::size_t>::max)();

    void ReadMaterials();
    std::filesystem::path GetTexturePath(std::size_t textureIndex) const;
    static DirectX::XMMATRIX GetLocalMatrix(const Json& node);
    void ReadNode(
        std::size_t nodeIndex,
        const DirectX::XMMATRIX& parentMatrix,
        int depth);
    std::size_t GetMaterialIndex(const Json& primitive);

    GltfFile file_;
    const Json& document_;
    ModelData model_;
    // 재질의 기본 색이며 model_.materials와 같은 순서입니다.
    std::vector<DirectX::XMFLOAT4> baseColors_;
    std::size_t defaultMaterialIndex_ = InvalidIndex;
};
