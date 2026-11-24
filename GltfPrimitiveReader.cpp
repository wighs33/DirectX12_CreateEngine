#include "GltfPrimitiveReader.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    // glTF 2.0 명세의 primitive.mode 값입니다.
    constexpr int PrimitiveModeTriangles = 4;
}

GltfPrimitiveReader::GltfPrimitiveReader(const GltfFile& file)
    : file_(file)
{
}

ModelPartData GltfPrimitiveReader::Read(
    const Json& primitive,
    const DirectX::XMMATRIX& worldMatrix,
    const DirectX::XMFLOAT4& baseColor) const
{
    if (primitive.value("mode", PrimitiveModeTriangles) !=
        PrimitiveModeTriangles)
    {
        throw std::runtime_error("삼각형이 아닌 도형은 지원하지 않습니다");
    }

    const Json& attributes = primitive.at("attributes");
    ModelPartData part;
    ReadVertices(attributes, worldMatrix, baseColor, part);
    ReadIndices(primitive, part);

    // 크기가 음수인 노드는 이미 한 번 뒤집혀 있습니다.
    const bool isMirrored = DirectX::XMVectorGetX(
        DirectX::XMMatrixDeterminant(worldMatrix)) < 0.0f;
    if (!isMirrored)
    {
        FlipWinding(part);
    }
    if (attributes.find("NORMAL") == attributes.end())
    {
        CalculateNormals(part);
    }
    return part;
}

bool GltfPrimitiveReader::TryGetAttribute(
    const Json& attributes,
    const char* name,
    std::size_t vertexCount,
    std::size_t minimumComponentCount,
    GltfAccessor& view) const
{
    const auto attribute = attributes.find(name);
    if (attribute == attributes.end())
    {
        return false;
    }

    view = file_.GetAccessor(attribute->get<std::size_t>());
    if (view.count != vertexCount ||
        view.componentCount < minimumComponentCount)
    {
        throw std::runtime_error(
            std::string("정점 값의 개수가 맞지 않습니다: ") + name);
    }

    return true;
}

void GltfPrimitiveReader::ReadVertices(
    const Json& attributes,
    const DirectX::XMMATRIX& worldMatrix,
    const DirectX::XMFLOAT4& baseColor,
    ModelPartData& part) const
{
    const GltfAccessor positions =
        file_.GetAccessor(attributes.at("POSITION").get<std::size_t>());
    if (positions.componentCount != 3 ||
        positions.count > (std::numeric_limits<std::uint32_t>::max)())
    {
        throw std::runtime_error("POSITION 형식이 올바르지 않습니다");
    }

    GltfAccessor normals;
    GltfAccessor uvs;
    GltfAccessor colors;
    const bool hasNormals = TryGetAttribute(
        attributes, "NORMAL", positions.count, 3, normals);
    const bool hasUvs = TryGetAttribute(
        attributes, "TEXCOORD_0", positions.count, 2, uvs);
    const bool hasColors = TryGetAttribute(
        attributes, "COLOR_0", positions.count, 3, colors);

    const DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixInverse(nullptr, worldMatrix));

    part.vertices.resize(positions.count);
    for (std::size_t index = 0; index < positions.count; ++index)
    {
        Vertex& vertex = part.vertices[index];

        DirectX::XMStoreFloat3(
            &vertex.position,
            DirectX::XMVector3TransformCoord(
                DirectX::XMVectorSet(
                    GltfFile::ReadFloat(positions, index, 0),
                    GltfFile::ReadFloat(positions, index, 1),
                    GltfFile::ReadFloat(positions, index, 2),
                    1.0f),
                worldMatrix));
        // 오른손 좌표계의 Z를 뒤집어 이 엔진의 왼손 좌표계로 옮깁니다.
        vertex.position.z = -vertex.position.z;

        if (hasNormals)
        {
            DirectX::XMStoreFloat3(
                &vertex.normal,
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3TransformNormal(
                        DirectX::XMVectorSet(
                            GltfFile::ReadFloat(normals, index, 0),
                            GltfFile::ReadFloat(normals, index, 1),
                            GltfFile::ReadFloat(normals, index, 2),
                            0.0f),
                        normalMatrix)));
            vertex.normal.z = -vertex.normal.z;
        }
        else
        {
            vertex.normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        vertex.uv = hasUvs
            ? DirectX::XMFLOAT2(
                GltfFile::ReadFloat(uvs, index, 0),
                GltfFile::ReadFloat(uvs, index, 1))
            : DirectX::XMFLOAT2(0.0f, 0.0f);

        // 재질의 기본 색은 정점 색에 미리 곱해 Shader가 하나의 색만 다루게 합니다.
        vertex.color = baseColor;
        if (hasColors)
        {
            vertex.color.x *= GltfFile::ReadFloat(colors, index, 0);
            vertex.color.y *= GltfFile::ReadFloat(colors, index, 1);
            vertex.color.z *= GltfFile::ReadFloat(colors, index, 2);
            if (colors.componentCount == 4)
            {
                vertex.color.w *= GltfFile::ReadFloat(colors, index, 3);
            }
        }
    }
}

void GltfPrimitiveReader::ReadIndices(
    const Json& primitive,
    ModelPartData& part) const
{
    const auto indices = primitive.find("indices");
    if (indices == primitive.end())
    {
        part.indices.resize(part.vertices.size());
        for (std::size_t index = 0; index < part.indices.size(); ++index)
        {
            part.indices[index] = static_cast<std::uint32_t>(index);
        }
    }
    else
    {
        const GltfAccessor view =
            file_.GetAccessor(indices->get<std::size_t>());
        part.indices.resize(view.count);
        for (std::size_t index = 0; index < view.count; ++index)
        {
            part.indices[index] = GltfFile::ReadIndex(view, index);
            if (part.indices[index] >= part.vertices.size())
            {
                throw std::runtime_error("인덱스가 정점 범위를 벗어났습니다");
            }
        }
    }

    if (part.indices.empty() || part.indices.size() % 3 != 0)
    {
        throw std::runtime_error("삼각형 인덱스 개수가 올바르지 않습니다");
    }
}

void GltfPrimitiveReader::FlipWinding(ModelPartData& part)
{
    for (std::size_t index = 0; index + 2 < part.indices.size(); index += 3)
    {
        std::swap(part.indices[index + 1], part.indices[index + 2]);
    }
}

void GltfPrimitiveReader::CalculateNormals(ModelPartData& part)
{
    for (std::size_t index = 0; index < part.indices.size(); index += 3)
    {
        Vertex& first = part.vertices[part.indices[index]];
        Vertex& second = part.vertices[part.indices[index + 1]];
        Vertex& third = part.vertices[part.indices[index + 2]];
        const DirectX::XMVECTOR origin =
            DirectX::XMLoadFloat3(&first.position);
        const DirectX::XMVECTOR faceNormal = DirectX::XMVector3Cross(
            DirectX::XMVectorSubtract(
                DirectX::XMLoadFloat3(&second.position), origin),
            DirectX::XMVectorSubtract(
                DirectX::XMLoadFloat3(&third.position), origin));
        for (Vertex* const vertex : { &first, &second, &third })
        {
            DirectX::XMStoreFloat3(
                &vertex->normal,
                DirectX::XMVectorAdd(
                    DirectX::XMLoadFloat3(&vertex->normal),
                    faceNormal));
        }
    }

    for (Vertex& vertex : part.vertices)
    {
        DirectX::XMStoreFloat3(
            &vertex.normal,
            DirectX::XMVector3Normalize(
                DirectX::XMLoadFloat3(&vertex.normal)));
    }
}
