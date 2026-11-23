#pragma once

#include "CreatePanel.h"
#include "EditorCameraController.h"
#include "EditorSelection.h"
#include "EngineGuideWindow.h"
#include "TransformGizmo.h"

#include <d3d12.h>

class Camera;
class ResourceManager;
class Scene;

// 편집기의 부분들을 정해진 순서로 부릅니다.
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
    void ProcessDeleteHotkey();
    void DrawPanels();
    void ProcessPicking(const D3D12_VIEWPORT& viewport);

    // 아래 대상의 수명은 D3D12Context가 관리합니다.
    HWND window_ = nullptr;
    Scene* scene_ = nullptr;
    Camera* camera_ = nullptr;
    ResourceManager* resourceManager_ = nullptr;

    EditorSelection selection_;
    EditorCameraController cameraController_;
    TransformGizmo gizmo_;
    CreatePanel createPanel_;
    EngineGuideWindow guideWindow_;
    bool deleteKeyWasDown_ = false;
};
