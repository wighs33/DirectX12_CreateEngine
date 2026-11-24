#include "SceneSerializer.h"

#include "GameObject.h"
#include "Model.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SceneData.h"
#include "SceneFileReader.h"
#include "SceneFileWriter.h"

#include <DirectXMath.h>

#include <cmath>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace
{
    // 이미 있는 모델을 먼저 찾고, 없으면 프로젝트 안의 glTF 경로로 보고 읽습니다.
    const Model* ResolveModel(
        ResourceManager& resourceManager,
        const std::string& modelId)
    {
        if (const Model* const model = resourceManager.FindModel(modelId))
        {
            return model;
        }

        const std::filesystem::path path = std::filesystem::u8path(modelId);
        if (path.extension() != ".gltf" || path.is_absolute())
        {
            return nullptr;
        }

        return resourceManager.LoadModel(path);
    }

    bool IsFiniteFloat3(const DirectX::XMFLOAT3& value)
    {
        return std::isfinite(value.x) &&
            std::isfinite(value.y) &&
            std::isfinite(value.z);
    }
}

bool SceneSerializer::SaveScene(
    const Scene& scene,
    const std::filesystem::path& filePath)
{
    try
    {
        SceneFileWriter::Write(CreateData(scene), filePath);
        std::printf("Scene saved: %s\n", filePath.string().c_str());
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "Scene save failed (%s): %s\n",
            filePath.string().c_str(),
            exception.what());
        return false;
    }
}

bool SceneSerializer::LoadScene(
    Scene& scene,
    ResourceManager& resourceManager,
    const std::filesystem::path& filePath)
{
    std::error_code existenceError;
    const bool fileExists = std::filesystem::exists(filePath, existenceError);
    if (!existenceError && !fileExists)
    {
        std::fprintf(
            stderr,
            "Scene file not found: %s\n",
            filePath.string().c_str());
        return false;
    }

    try
    {
        Scene loadedScene =
            CreateScene(SceneFileReader::Read(filePath), resourceManager);
        scene = std::move(loadedScene);
        std::printf("Scene loaded: %s\n", filePath.string().c_str());
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "Scene load failed (%s): %s\n",
            filePath.string().c_str(),
            exception.what());
        return false;
    }
}

SceneData SceneSerializer::CreateData(const Scene& scene)
{
    SceneData data;
    for (const GameObject& gameObject : scene.GetGameObjects())
    {
        const Model* const model = gameObject.GetModel();
        if (model == nullptr)
        {
            throw std::runtime_error(
                "the scene contains an object without a model");
        }

        const Transform& transform = gameObject.GetTransform();
        const DirectX::XMFLOAT3& rotation = transform.GetRotation();
        if (!IsFiniteFloat3(transform.GetPosition()) ||
            !IsFiniteFloat3(rotation) ||
            !IsFiniteFloat3(transform.GetScale()))
        {
            throw std::runtime_error(
                "the scene contains a non-finite transform value");
        }

        SceneObjectData object;
        object.name = gameObject.GetName();
        object.modelId = model->GetId();
        object.position = transform.GetPosition();
        object.rotationDegrees = DirectX::XMFLOAT3(
            DirectX::XMConvertToDegrees(rotation.x),
            DirectX::XMConvertToDegrees(rotation.y),
            DirectX::XMConvertToDegrees(rotation.z));
        object.scale = transform.GetScale();
        data.objects.push_back(std::move(object));
    }
    return data;
}

Scene SceneSerializer::CreateScene(
    const SceneData& data,
    ResourceManager& resourceManager)
{
    Scene scene;
    for (const SceneObjectData& object : data.objects)
    {
        const Model* const model = ResolveModel(resourceManager, object.modelId);
        if (model == nullptr)
        {
            throw std::runtime_error(
                "the model is not available: " + object.modelId);
        }

        GameObject gameObject(model);
        gameObject.SetName(object.name);

        Transform& transform = gameObject.GetTransform();
        transform.SetPosition(
            object.position.x,
            object.position.y,
            object.position.z);
        transform.SetRotation(
            DirectX::XMConvertToRadians(object.rotationDegrees.x),
            DirectX::XMConvertToRadians(object.rotationDegrees.y),
            DirectX::XMConvertToRadians(object.rotationDegrees.z));
        transform.SetScale(object.scale.x, object.scale.y, object.scale.z);

        scene.AddGameObject(std::move(gameObject));
    }
    return scene;
}
