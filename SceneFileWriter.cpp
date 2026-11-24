#include "SceneFileWriter.h"

#include "SceneData.h"

#include "json.hpp"

#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace
{
    using Json = nlohmann::json;

    Json CreateObjectJson(const SceneObjectData& object)
    {
        const DirectX::XMFLOAT3& position = object.position;
        const DirectX::XMFLOAT3& rotation = object.rotationDegrees;
        const DirectX::XMFLOAT3& scale = object.scale;

        Json objectJson = Json::object();
        objectJson["name"] = object.name;
        objectJson["model"] = object.modelId;
        objectJson["transform"] =
        {
            { "position", { position.x, position.y, position.z } },
            { "rotationDegrees", { rotation.x, rotation.y, rotation.z } },
            { "scale", { scale.x, scale.y, scale.z } }
        };
        return objectJson;
    }

    void CreateParentDirectory(const std::filesystem::path& filePath)
    {
        const std::filesystem::path directory = filePath.parent_path();
        if (directory.empty())
        {
            return;
        }

        std::error_code directoryError;
        std::filesystem::create_directories(directory, directoryError);
        if (directoryError)
        {
            throw std::runtime_error(
                "could not create the scene directory: " +
                    directoryError.message());
        }
    }
}

void SceneFileWriter::Write(
    const SceneData& data,
    const std::filesystem::path& filePath)
{
    Json sceneJson = Json::object();
    sceneJson["version"] = SceneFileFormat::CurrentVersion;
    sceneJson["objects"] = Json::array();
    for (const SceneObjectData& object : data.objects)
    {
        sceneJson["objects"].push_back(CreateObjectJson(object));
    }

    CreateParentDirectory(filePath);

    std::ofstream output(filePath, std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error("could not open the file");
    }

    output << sceneJson.dump(4) << '\n';
    output.flush();
    if (!output.good())
    {
        throw std::runtime_error("could not write the file");
    }
}
