#include "SceneFileReader.h"

#include "json.hpp"

#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    bool TryReadFloat3(
        const Json& value,
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
            const Json& component = value[componentIndex];
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

    // file이 기대하는 형식이 아니면 예외를 던지고 version을 돌려줍니다.
    int ReadVersion(const Json& sceneJson)
    {
        const auto version = sceneJson.find("version");
        if (version == sceneJson.end() || !version->is_number_integer())
        {
            throw std::runtime_error("version is missing or is not an integer");
        }

        const int fileVersion = version->get<int>();
        if (fileVersion != SceneFileFormat::CurrentVersion &&
            fileVersion != SceneFileFormat::PrimitiveOnlyVersion)
        {
            throw std::runtime_error("unsupported scene version");
        }
        return fileVersion;
    }

    std::string ReadString(
        const Json& objectJson,
        const std::string& objectPrefix,
        const char* key)
    {
        const auto value = objectJson.find(key);
        if (value == objectJson.end() || !value->is_string())
        {
            throw std::runtime_error(
                objectPrefix + key + " is missing or is not a string");
        }
        return value->get<std::string>();
    }

    DirectX::XMFLOAT3 ReadFloat3(
        const Json& transformJson,
        const std::string& objectPrefix,
        const char* key)
    {
        DirectX::XMFLOAT3 result{};
        const auto value = transformJson.find(key);
        if (value == transformJson.end() || !TryReadFloat3(*value, result))
        {
            throw std::runtime_error(
                objectPrefix + key + " must contain three numbers");
        }
        return result;
    }

    SceneObjectData ReadObject(
        const Json& objectJson,
        const std::string& objectPrefix,
        int fileVersion)
    {
        if (!objectJson.is_object())
        {
            throw std::runtime_error(objectPrefix + "entry is not an object");
        }

        SceneObjectData object;
        object.name = ReadString(objectJson, objectPrefix, "name");
        object.modelId = ReadString(
            objectJson,
            objectPrefix,
            fileVersion == SceneFileFormat::PrimitiveOnlyVersion
                ? "primitive"
                : "model");

        const auto transform = objectJson.find("transform");
        if (transform == objectJson.end() || !transform->is_object())
        {
            throw std::runtime_error(
                objectPrefix + "transform is missing or is not an object");
        }

        object.position = ReadFloat3(*transform, objectPrefix, "position");
        object.rotationDegrees =
            ReadFloat3(*transform, objectPrefix, "rotationDegrees");
        object.scale = ReadFloat3(*transform, objectPrefix, "scale");
        return object;
    }
}

SceneData SceneFileReader::Read(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        throw std::runtime_error("could not open the file");
    }

    Json sceneJson;
    input >> sceneJson;
    if (!sceneJson.is_object())
    {
        throw std::runtime_error("the root must be an object");
    }

    const int fileVersion = ReadVersion(sceneJson);

    const auto objects = sceneJson.find("objects");
    if (objects == sceneJson.end() || !objects->is_array())
    {
        throw std::runtime_error("objects is missing or is not an array");
    }

    SceneData data;
    data.objects.reserve(objects->size());
    for (std::size_t objectIndex = 0;
         objectIndex < objects->size();
         ++objectIndex)
    {
        data.objects.push_back(ReadObject(
            (*objects)[objectIndex],
            "object " + std::to_string(objectIndex) + ": ",
            fileVersion));
    }
    return data;
}
