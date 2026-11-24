#include "GltfModelReader.h"

#include "GltfPrimitiveReader.h"

#include <stdexcept>
#include <utility>

GltfModelReader::GltfModelReader(const std::filesystem::path& filePath)
    : file_(filePath),
      document_(file_.GetDocument())
{
}

ModelData GltfModelReader::Read()
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

void GltfModelReader::ReadMaterials()
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

std::filesystem::path GltfModelReader::GetTexturePath(
    std::size_t textureIndex) const
{
    const std::size_t imageIndex = document_.at("textures")
        .at(textureIndex)
        .at("source")
        .get<std::size_t>();
    const Json& image = document_.at("images").at(imageIndex);
    return file_.ResolveUri(image.at("uri").get<std::string>());
}

DirectX::XMMATRIX GltfModelReader::GetLocalMatrix(const Json& node)
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

void GltfModelReader::ReadNode(
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
            const std::size_t materialIndex = GetMaterialIndex(primitive);
            ModelPartData part = GltfPrimitiveReader(file_).Read(
                primitive,
                isSkinned ? DirectX::XMMatrixIdentity() : worldMatrix,
                baseColors_[materialIndex]);
            part.name = name;
            part.materialIndex = materialIndex;
            model_.parts.push_back(std::move(part));
        }
    }

    for (const Json& childIndex : node.value("children", Json::array()))
    {
        ReadNode(childIndex.get<std::size_t>(), worldMatrix, depth + 1);
    }
}

std::size_t GltfModelReader::GetMaterialIndex(const Json& primitive)
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
