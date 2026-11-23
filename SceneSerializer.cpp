#include "SceneSerializer.h"

#include "GameObject.h"
#include "Model.h"
#include "ResourceManager.h"
#include "Scene.h"

#include "json.hpp"

#include <DirectXMath.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
    constexpr int SceneFormatVersion = 1;

    struct SerializedObject
    {
        std::string name;
        std::string modelId;
        DirectX::XMFLOAT3 position{};
        DirectX::XMFLOAT3 rotationDegrees{};
        DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
    };

    bool ReportSaveError(
        const std::filesystem::path& filePath,
        const std::string& reason)
    {
        std::fprintf(
            stderr,
            "Scene save failed (%s): %s\n",
            filePath.string().c_str(),
            reason.c_str());
        return false;
    }

    bool ReportLoadError(
        const std::filesystem::path& filePath,
        const std::string& reason)
    {
        std::fprintf(
            stderr,
            "Scene load failed (%s): %s\n",
            filePath.string().c_str(),
            reason.c_str());
        return false;
    }

    bool IsPrimitiveName(const std::string& name)
    {
        return name == "Cube" || name == "Sphere";
    }

    bool IsFiniteFloat3(const DirectX::XMFLOAT3& value)
    {
        return std::isfinite(value.x) &&
            std::isfinite(value.y) &&
            std::isfinite(value.z);
    }

    bool TryReadFloat3(
        const nlohmann::json& value,
        DirectX::XMFLOAT3& result)
    {
        if (!value.is_array() || value.size() != 3)
        {
            return false;
        }

        float components[3]{};
        for (std::size_t componentIndex = 0;
             componentIndex < 3;
             ++componentIndex)
        {
            const nlohmann::json& component = value[componentIndex];
            if (!component.is_number())
            {
                return false;
            }

            const double number = component.get<double>();
            const double maximumFloat =
                static_cast<double>((std::numeric_limits<float>::max)());
            if (!std::isfinite(number) ||
                number < -maximumFloat ||
                number > maximumFloat)
            {
                return false;
            }

            components[componentIndex] = static_cast<float>(number);
        }

        result = DirectX::XMFLOAT3(
            components[0],
            components[1],
            components[2]);
        return true;
    }
}

bool SceneSerializer::SaveScene(
    const Scene& scene,
    const std::filesystem::path& filePath)
{
    try
    {
        nlohmann::json sceneJson = nlohmann::json::object();
        sceneJson["version"] = SceneFormatVersion;
        sceneJson["objects"] = nlohmann::json::array();

        for (const GameObject& gameObject : scene.GetGameObjects())
        {
            const Model* const model = gameObject.GetModel();
            if (model == nullptr || !IsPrimitiveName(model->GetId()))
            {
                return ReportSaveError(
                    filePath,
                    "the scene contains an unsupported primitive type");
            }

            const Transform& transform = gameObject.GetTransform();
            const DirectX::XMFLOAT3& position = transform.GetPosition();
            const DirectX::XMFLOAT3& rotation = transform.GetRotation();
            const DirectX::XMFLOAT3& scale = transform.GetScale();
            if (!IsFiniteFloat3(position) ||
                !IsFiniteFloat3(rotation) ||
                !IsFiniteFloat3(scale))
            {
                return ReportSaveError(
                    filePath,
                    "the scene contains a non-finite transform value");
            }

            nlohmann::json objectJson = nlohmann::json::object();
            objectJson["name"] = gameObject.GetName();
            objectJson["primitive"] = model->GetId();
            objectJson["transform"] =
            {
                {
                    "position",
                    { position.x, position.y, position.z }
                },
                {
                    "rotationDegrees",
                    {
                        DirectX::XMConvertToDegrees(rotation.x),
                        DirectX::XMConvertToDegrees(rotation.y),
                        DirectX::XMConvertToDegrees(rotation.z)
                    }
                },
                {
                    "scale",
                    { scale.x, scale.y, scale.z }
                }
            };
            sceneJson["objects"].push_back(std::move(objectJson));
        }

        const std::filesystem::path directory = filePath.parent_path();
        if (!directory.empty())
        {
            std::error_code directoryError;
            std::filesystem::create_directories(directory, directoryError);
            if (directoryError)
            {
                return ReportSaveError(
                    filePath,
                    "could not create the scene directory: " +
                        directoryError.message());
            }
        }

        std::ofstream output(filePath, std::ios::out | std::ios::trunc);
        if (!output.is_open())
        {
            return ReportSaveError(filePath, "could not open the file");
        }

        output << sceneJson.dump(4) << '\n';
        output.flush();
        if (!output.good())
        {
            return ReportSaveError(filePath, "could not write the file");
        }

        std::printf("Scene saved: %s\n", filePath.string().c_str());
        return true;
    }
    catch (const std::exception& exception)
    {
        return ReportSaveError(filePath, exception.what());
    }
}

bool SceneSerializer::LoadScene(
    Scene& scene,
    ResourceManager& resourceManager,
    const std::filesystem::path& filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        std::error_code existenceError;
        const bool fileExists =
            std::filesystem::exists(filePath, existenceError);
        if (!existenceError && !fileExists)
        {
            std::fprintf(
                stderr,
                "Scene file not found: %s\n",
                filePath.string().c_str());
            return false;
        }

        return ReportLoadError(filePath, "could not open the file");
    }

    try
    {
        nlohmann::json sceneJson;
        input >> sceneJson;

        if (!sceneJson.is_object())
        {
            return ReportLoadError(filePath, "the root must be an object");
        }

        const auto version = sceneJson.find("version");
        if (version == sceneJson.end() || !version->is_number_integer())
        {
            return ReportLoadError(
                filePath,
                "version is missing or is not an integer");
        }
        if (*version != SceneFormatVersion)
        {
            return ReportLoadError(
                filePath,
                "unsupported scene version");
        }

        const auto objects = sceneJson.find("objects");
        if (objects == sceneJson.end() || !objects->is_array())
        {
            return ReportLoadError(
                filePath,
                "objects is missing or is not an array");
        }

        std::vector<SerializedObject> serializedObjects;
        serializedObjects.reserve(objects->size());
        for (std::size_t objectIndex = 0;
             objectIndex < objects->size();
             ++objectIndex)
        {
            const nlohmann::json& objectJson = (*objects)[objectIndex];
            const std::string objectPrefix =
                "object " + std::to_string(objectIndex) + ": ";
            if (!objectJson.is_object())
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix + "entry is not an object");
            }

            const auto name = objectJson.find("name");
            if (name == objectJson.end() || !name->is_string())
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix + "name is missing or is not a string");
            }

            const auto primitive = objectJson.find("primitive");
            if (primitive == objectJson.end() || !primitive->is_string())
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix +
                        "primitive is missing or is not a string");
            }

            const auto transform = objectJson.find("transform");
            if (transform == objectJson.end() || !transform->is_object())
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix +
                        "transform is missing or is not an object");
            }

            SerializedObject serializedObject{};
            serializedObject.name = name->get<std::string>();
            serializedObject.modelId = primitive->get<std::string>();
            if (!IsPrimitiveName(serializedObject.modelId))
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix + "primitive must be Cube or Sphere");
            }

            const auto position = transform->find("position");
            if (position == transform->end() ||
                !TryReadFloat3(*position, serializedObject.position))
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix + "position must contain three numbers");
            }

            const auto rotation = transform->find("rotationDegrees");
            if (rotation == transform->end() ||
                !TryReadFloat3(*rotation, serializedObject.rotationDegrees))
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix +
                        "rotationDegrees must contain three numbers");
            }

            const auto scale = transform->find("scale");
            if (scale == transform->end() ||
                !TryReadFloat3(*scale, serializedObject.scale))
            {
                return ReportLoadError(
                    filePath,
                    objectPrefix + "scale must contain three numbers");
            }

            serializedObjects.push_back(std::move(serializedObject));
        }

        Scene loadedScene;
        for (const SerializedObject& serializedObject : serializedObjects)
        {
            const Model* const model =
                resourceManager.FindModel(serializedObject.modelId);
            if (model == nullptr)
            {
                return ReportLoadError(
                    filePath,
                    "the model is not available: " + serializedObject.modelId);
            }

            GameObject gameObject(model);
            gameObject.SetName(serializedObject.name);

            Transform& transform = gameObject.GetTransform();
            transform.SetPosition(
                serializedObject.position.x,
                serializedObject.position.y,
                serializedObject.position.z);
            transform.SetRotation(
                DirectX::XMConvertToRadians(
                    serializedObject.rotationDegrees.x),
                DirectX::XMConvertToRadians(
                    serializedObject.rotationDegrees.y),
                DirectX::XMConvertToRadians(
                    serializedObject.rotationDegrees.z));
            transform.SetScale(
                serializedObject.scale.x,
                serializedObject.scale.y,
                serializedObject.scale.z);

            loadedScene.AddGameObject(std::move(gameObject));
        }

        // 모든 Parsing, Validation과 임시 Scene 구성이 끝난 뒤에만 현재 Scene을 교체합니다.
        scene = std::move(loadedScene);
        std::printf("Scene loaded: %s\n", filePath.string().c_str());
        return true;
    }
    catch (const nlohmann::json::exception& exception)
    {
        return ReportLoadError(filePath, exception.what());
    }
    catch (const std::exception& exception)
    {
        return ReportLoadError(filePath, exception.what());
    }
}
