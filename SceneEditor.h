#pragma once

#include "EngineGuideWindow.h"

#include <d3d12.h>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

class Camera;
class Model;
class ResourceManager;
class Scene;
struct Ray;

// Panel과 Gizmo, 클릭 선택, 카메라 조작처럼 장면을 편집하는 일을 맡습니다.
// 장면과 카메라를 소유하지 않으며 그리기 명령도 기록하지 않습니다.
class SceneEditor final
{
public:
    SceneEditor() = default;
    ~SceneEditor() = default;

    SceneEditor(const SceneEditor&) = delete;
    SceneEditor& operator=(const SceneEditor&) = delete;

    // ImGui Context를 만든 뒤 첫 Frame 전에 한 번 호출합니다.
    void Initialize(
        HWND window,
        Scene& scene,
        Camera& camera,
        ResourceManager& resourceManager);

    // ImGui Frame을 시작한 뒤 입력을 해석하고 편집 UI를 그립니다.
    void Update(float deltaTime, const D3D12_VIEWPORT& viewport);

private:
    enum class GizmoOperation
    {
        Translate,
        Rotate,
        Scale
    };

    static constexpr std::size_t InvalidObjectIndex =
        (std::numeric_limits<std::size_t>::max)();

    void UpdateCameraLookState();
    void UpdateCamera(float deltaTime);
    void ProcessDeleteHotkey();
    void ProcessPicking();

    void DrawEditorUI();
    void DrawModelPicker();
    void AddSceneObject(const Model* model, const std::string& baseName);
    std::string CreateUniqueObjectName(const std::string& baseName) const;
    void ProcessGizmoHotkeys();
    void DrawTransformGizmo();
    void ProcessObjectDeletionRequest();
    bool CreatePickingRay(const POINT& mousePosition, Ray& ray) const;
    void PerformViewportPicking(const POINT& mousePosition);

    // 아래 대상의 수명은 D3D12Context가 관리합니다.
    HWND window_ = nullptr;
    Scene* scene_ = nullptr;
    Camera* camera_ = nullptr;
    ResourceManager* resourceManager_ = nullptr;

    D3D12_VIEWPORT viewport_{};

    // 물음표 Button과 엔진 도감 창입니다.
    EngineGuideWindow engineGuideWindow_;
    // Create Panel에서 고를 수 있는 Assets/Models 안의 glTF 파일입니다.
    std::vector<std::filesystem::path> modelFiles_;
    std::size_t selectedModelFile_ = 0;

    // Camera 입력을 해석하는 상태이며 RMB를 새로 누른 경우에만 활성화됩니다.
    bool isCameraLookActive_ = false;
    GizmoOperation gizmoOperation_ = GizmoOperation::Translate;
    // Selection은 Scene Object 자체가 아닌 Editor가 관리하며 vector 재배치에 안전한 Index를 사용합니다.
    std::size_t selectedObjectIndex_ = InvalidObjectIndex;
    bool deleteObjectRequested_ = false;
    bool deleteKeyWasDown_ = false;
};
