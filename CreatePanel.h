#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class EditorSelection;
class EngineGuideWindow;
class Model;
class ResourceManager;
class Scene;

// 도형과 모델을 장면에 넣고 장면을 저장하거나 불러오는 패널입니다.
class CreatePanel final
{
public:
    void Initialize();

    void Draw(
        Scene& scene,
        ResourceManager& resourceManager,
        EditorSelection& selection,
        EngineGuideWindow& guideWindow);

private:
    void DrawModelPicker(
        Scene& scene,
        ResourceManager& resourceManager,
        EditorSelection& selection);
    static void DrawSceneButtons(
        Scene& scene,
        ResourceManager& resourceManager,
        EditorSelection& selection);
    // 새 물체를 넣고 바로 고릅니다.
    static void AddObject(
        const Model* model,
        const std::string& baseName,
        Scene& scene,
        EditorSelection& selection);

    // Assets/Models 안에서 찾은 glTF 파일입니다.
    std::vector<std::filesystem::path> modelFiles_;
    std::size_t selectedModelFile_ = 0;
};
