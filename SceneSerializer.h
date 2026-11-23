#pragma once

#include <filesystem>

class ResourceManager;
class Scene;

// Runtime Scene과 사람이 읽을 수 있는 JSON File 사이의 변환만 담당합니다.
class SceneSerializer final
{
public:
    SceneSerializer() = delete;

    static bool SaveScene(
        const Scene& scene,
        const std::filesystem::path& filePath);
    static bool LoadScene(
        Scene& scene,
        ResourceManager& resourceManager,
        const std::filesystem::path& filePath);
};
