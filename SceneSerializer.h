#pragma once

#include <filesystem>

class ResourceManager;
class Scene;
struct SceneData;

// 실행 중인 Scene과 파일에 적을 SceneData 사이를 바꿉니다.
class SceneSerializer final
{
public:
    SceneSerializer() = delete;

    static bool SaveScene(
        const Scene& scene,
        const std::filesystem::path& filePath);

    // 파일을 모두 읽고 모델까지 찾은 뒤에만 현재 Scene을 교체합니다.
    static bool LoadScene(
        Scene& scene,
        ResourceManager& resourceManager,
        const std::filesystem::path& filePath);

private:
    static SceneData CreateData(const Scene& scene);
    static Scene CreateScene(
        const SceneData& data,
        ResourceManager& resourceManager);
};
