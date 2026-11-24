#pragma once

#include "GltfFile.h"
#include "ModelData.h"

#include <DirectXMath.h>

#include <cstddef>

// glTF의 조각 하나를 이 엔진의 정점 형식과 왼손 좌표계로 바꿉니다.
// 실패하면 이유를 담은 예외를 던집니다.
class GltfPrimitiveReader final
{
public:
    // file은 읽는 동안 살아 있어야 합니다.
    explicit GltfPrimitiveReader(const GltfFile& file);

    // baseColor는 재질의 기본 색이며 정점 색에 미리 곱합니다.
    ModelPartData Read(
        const nlohmann::json& primitive,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMFLOAT4& baseColor) const;

private:
    using Json = nlohmann::json;

    // 이름에 해당하는 정점 값이 있으면 true를 반환하며 개수와 성분 수를 확인합니다.
    bool TryGetAttribute(
        const Json& attributes,
        const char* name,
        std::size_t vertexCount,
        std::size_t minimumComponentCount,
        GltfAccessor& view) const;
    void ReadVertices(
        const Json& attributes,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMFLOAT4& baseColor,
        ModelPartData& part) const;
    void ReadIndices(const Json& primitive, ModelPartData& part) const;

    // Z를 뒤집어 반대가 된 감기는 방향을 이 엔진에 맞춥니다.
    static void FlipWinding(ModelPartData& part);

    // 법선이 없는 파일은 면의 법선을 꼭짓점마다 더해 부드러운 법선을 만듭니다.
    static void CalculateNormals(ModelPartData& part);

    const GltfFile& file_;
};
