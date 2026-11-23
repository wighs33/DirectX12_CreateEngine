#include "ModelLoader.h"

#include "GltfFile.h"
#include "ModelData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    // glTF 2.0 명세의 primitive.mode 값입니다.
    constexpr int PrimitiveModeTriangles = 4;

    // GltfFile에서 꺼낸 값을 이 엔진의 정점과 재질로 바꿉니다.
    class GltfReader final
    {
    public:
        explicit GltfReader(const std::filesystem::path& filePath)
            : file_(filePath),
              document_(file_.GetDocument())
        {
        }

        ModelData Read()
        {
            ReadMaterials();

            const std::size_t sceneIndex =
                document_.value("scene", static_cast<std::size_t>(0));
            const Json& scene = document_.at("scenes").at(sceneIndex);
            for (const Json& nodeIndex : scene.at("nodes"))
            {
                ReadNode(
                    nodeIndex.get<std::size_t>(),
                    DirectX::XMMatrixIdentity(),
                    0);
            }

            if (model_.parts.empty())
            {
                throw std::runtime_error("그릴 삼각형이 없습니다");
            }

            return std::move(model_);
        }

    private:
        static constexpr int MaximumNodeDepth = 256;

        void ReadMaterials()
        {
            for (const Json& material :
                 document_.value("materials", Json::array()))
            {
                ModelMaterialData data;
                data.name = material.value("name", std::string());

                DirectX::XMFLOAT4 baseColor(1.0f, 1.0f, 1.0f, 1.0f);
                const auto pbr = material.find("pbrMetallicRoughness");
                if (pbr != material.end())
                {
                    const auto factor = pbr->find("baseColorFactor");
                    if (factor != pbr->end())
                    {
                        baseColor = DirectX::XMFLOAT4(
                            factor->at(0).get<float>(),
                            factor->at(1).get<float>(),
                            factor->at(2).get<float>(),
                            factor->at(3).get<float>());
                    }

                    const auto texture = pbr->find("baseColorTexture");
                    if (texture != pbr->end())
                    {
                        data.baseColorTexturePath = GetTexturePath(
                            texture->at("index").get<std::size_t>());
                    }
                }

                model_.materials.push_back(std::move(data));
                baseColors_.push_back(baseColor);
            }
        }

        std::filesystem::path GetTexturePath(std::size_t textureIndex) const
        {
            const std::size_t imageIndex = document_.at("textures")
                .at(textureIndex)
                .at("source")
                .get<std::size_t>();
            const Json& image = document_.at("images").at(imageIndex);
            return file_.ResolveUri(image.at("uri").get<std::string>());
        }

        static DirectX::XMMATRIX GetLocalMatrix(const Json& node)
        {
            // glTF의 열 벡터/열 우선 배열은 DirectXMath의 행 벡터/행 우선 메모리와 순서가 같습니다.
            const auto matrix = node.find("matrix");
            if (matrix != node.end())
            {
                float values[16]{};
                for (std::size_t index = 0; index < 16; ++index)
                {
                    values[index] = matrix->at(index).get<float>();
                }
                const DirectX::XMFLOAT4X4 stored(values);
                return DirectX::XMLoadFloat4x4(&stored);
            }

            const std::vector<float> scale =
                node.value("scale", std::vector<float>{ 1.0f, 1.0f, 1.0f });
            const std::vector<float> rotation =
                node.value("rotation", std::vector<float>{ 0.0f, 0.0f, 0.0f, 1.0f });
            const std::vector<float> translation =
                node.value("translation", std::vector<float>{ 0.0f, 0.0f, 0.0f });

            return DirectX::XMMatrixScaling(
                    scale.at(0), scale.at(1), scale.at(2)) *
                DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(
                    rotation.at(0), rotation.at(1), rotation.at(2), rotation.at(3))) *
                DirectX::XMMatrixTranslation(
                    translation.at(0), translation.at(1), translation.at(2));
        }

        void ReadNode(
            std::size_t nodeIndex,
            const DirectX::XMMATRIX& parentMatrix,
            int depth)
        {
            if (depth > MaximumNodeDepth)
            {
                throw std::runtime_error("노드 계층이 너무 깊습니다");
            }

            const Json& node = document_.at("nodes").at(nodeIndex);
            const DirectX::XMMATRIX worldMatrix =
                GetLocalMatrix(node) * parentMatrix;

            const auto mesh = node.find("mesh");
            if (mesh != node.end())
            {
                const Json& meshJson =
                    document_.at("meshes").at(mesh->get<std::size_t>());
                const std::string name = node.value(
                    "name",
                    meshJson.value("name", std::string("Part")));
                // 뼈에 묶인 메시의 자리는 뼈가 정하므로 명세에 따라 노드 행렬을 쓰지 않습니다.
                const bool isSkinned = node.find("skin") != node.end();
                for (const Json& primitive : meshJson.at("primitives"))
                {
                    ReadPrimitive(
                        primitive,
                        name,
                        isSkinned
                            ? DirectX::XMMatrixIdentity()
                            : worldMatrix);
                }
            }

            for (const Json& childIndex : node.value("children", Json::array()))
            {
                ReadNode(childIndex.get<std::size_t>(), worldMatrix, depth + 1);
            }
        }

        // 이름에 해당하는 정점 값이 있으면 true를 반환하며 개수와 성분 수를 확인합니다.
        bool TryGetAttribute(
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

        std::size_t GetMaterialIndex(const Json& primitive)
        {
            const auto material = primitive.find("material");
            if (material != primitive.end())
            {
                const std::size_t index = material->get<std::size_t>();
                if (index >= model_.materials.size())
                {
                    throw std::runtime_error("material 번호가 범위를 벗어났습니다");
                }
                return index;
            }

            // 재질이 없는 조각은 흰색 기본 재질 하나를 함께 씁니다.
            if (defaultMaterialIndex_ == InvalidIndex)
            {
                defaultMaterialIndex_ = model_.materials.size();
                model_.materials.push_back({ "Default", {} });
                baseColors_.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
            }
            return defaultMaterialIndex_;
        }

        void ReadPrimitive(
            const Json& primitive,
            const std::string& name,
            const DirectX::XMMATRIX& worldMatrix)
        {
            if (primitive.value("mode", PrimitiveModeTriangles) !=
                PrimitiveModeTriangles)
            {
                throw std::runtime_error("삼각형이 아닌 도형은 지원하지 않습니다");
            }

            const Json& attributes = primitive.at("attributes");
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

            ModelPartData part;
            part.name = name;
            part.materialIndex = GetMaterialIndex(primitive);
            const DirectX::XMFLOAT4& baseColor = baseColors_[part.materialIndex];

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

            ReadIndices(primitive, part);

            // Z를 뒤집으면 감기는 방향이 이 엔진과 반대가 됩니다. 크기가 음수인 노드는 이미 한 번 뒤집혀 있습니다.
            const bool isMirrored = DirectX::XMVectorGetX(
                DirectX::XMMatrixDeterminant(worldMatrix)) < 0.0f;
            if (!isMirrored)
            {
                for (std::size_t index = 0;
                     index + 2 < part.indices.size();
                     index += 3)
                {
                    std::swap(part.indices[index + 1], part.indices[index + 2]);
                }
            }

            if (!hasNormals)
            {
                CalculateNormals(part);
            }

            model_.parts.push_back(std::move(part));
        }

        void ReadIndices(const Json& primitive, ModelPartData& part) const
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

        // 법선이 없는 파일은 면의 법선을 꼭짓점마다 더해 부드러운 법선을 만듭니다.
        static void CalculateNormals(ModelPartData& part)
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

        static constexpr std::size_t InvalidIndex =
            (std::numeric_limits<std::size_t>::max)();

        GltfFile file_;
        const Json& document_;
        ModelData model_;
        std::vector<DirectX::XMFLOAT4> baseColors_;
        std::size_t defaultMaterialIndex_ = InvalidIndex;
    };
}

bool ModelLoader::LoadGltf(
    const std::filesystem::path& filePath,
    ModelData& modelData)
{
    try
    {
        modelData = GltfReader(filePath).Read();
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "glTF 파일을 읽지 못했습니다: %s (%s)\n",
            filePath.u8string().c_str(),
            exception.what());
        return false;
    }
}
