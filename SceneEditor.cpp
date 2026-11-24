#include "SceneEditor.h"

#include "Camera.h"
#include "HierarchyPanel.h"
#include "Input.h"
#include "InspectorPanel.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "ViewportHelpBar.h"
#include "ViewportPicker.h"

#include "imgui.h"

#include <cstddef>
#include <filesystem>

namespace
{
    const std::filesystem::path& GetGuideFilePath()
    {
        static const std::filesystem::path guideFilePath =
            std::filesystem::path("Assets") / "Guide" / "EngineGuide.json";
        return guideFilePath;
    }
}

void SceneEditor::Initialize(
    HWND window,
    Scene& scene,
    Camera& camera,
    ResourceManager& resourceManager)
{
    window_ = window;
    scene_ = &scene;
    camera_ = &camera;
    resourceManager_ = &resourceManager;

    guideWindow_.Initialize(GetGuideFilePath());
    createPanel_.Initialize();
}

void SceneEditor::Update(float deltaTime, const D3D12_VIEWPORT& viewport)
{
    Input::Update(window_);
    cameraController_.UpdateLookState(window_);
    ProcessDeleteHotkey();
    gizmo_.ProcessHotkeys(cameraController_.IsLooking());

    // UI 편집을 처리한 뒤 삭제로 Selection을 정리하고 마지막에 Gizmo를 갱신합니다.
    DrawPanels();
    selection_.ProcessDeleteRequest(*scene_);
    gizmo_.Draw(
        selection_.GetSelected(*scene_),
        *camera_,
        viewport,
        cameraController_.IsLooking());

    cameraController_.MoveCamera(*camera_, deltaTime);
    ProcessPicking(viewport);
}

void SceneEditor::ProcessDeleteHotkey()
{
    const ImGuiIO& io = ImGui::GetIO();

    // Delete 단축키는 눌린 첫 Frame에서만 요청하고 UI 입력 또는 Camera Look 중에는 무시합니다.
    const bool isDeleteKeyDown = Input::IsKeyDown(VK_DELETE);
    const bool wasDeleteKeyPressed = isDeleteKeyDown && !deleteKeyWasDown_;
    deleteKeyWasDown_ = isDeleteKeyDown;
    if (wasDeleteKeyPressed &&
        !io.WantCaptureKeyboard &&
        !cameraController_.IsLooking())
    {
        selection_.RequestDelete();
    }
}

void SceneEditor::DrawPanels()
{
    gizmo_.DrawToolbar(guideWindow_);
    createPanel_.Draw(*scene_, *resourceManager_, selection_, guideWindow_);
    HierarchyPanel::Draw(*scene_, selection_, guideWindow_);
    InspectorPanel::Draw(*scene_, selection_, guideWindow_);
    stylePanel_.Draw(
        *scene_,
        selection_,
        resourceManager_->GetStyles(),
        guideWindow_);
    ViewportHelpBar::Draw(guideWindow_);
    guideWindow_.Draw();
}

void SceneEditor::ProcessPicking(const D3D12_VIEWPORT& viewport)
{
    const bool isGizmoInteraction =
        selection_.GetSelected(*scene_) != nullptr &&
        TransformGizmo::IsInteracting();

    POINT mousePosition{};
    if (!Input::WasLeftMouseButtonPressed() ||
        ImGui::GetIO().WantCaptureMouse ||
        isGizmoInteraction ||
        cameraController_.IsLooking() ||
        Input::IsRightMouseButtonDown() ||
        !Input::GetMousePosition(mousePosition) ||
        !ViewportPicker::IsInside(viewport, mousePosition))
    {
        return;
    }

    // Hit가 없으면 기존 Selection도 해제하여 Hierarchy와 Inspector가 같은 상태를 공유합니다.
    std::size_t objectIndex = 0;
    if (ViewportPicker::Pick(
            *scene_,
            *camera_,
            viewport,
            mousePosition,
            objectIndex))
    {
        selection_.Select(objectIndex);
    }
    else
    {
        selection_.Clear();
    }
}
