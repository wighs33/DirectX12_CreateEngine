#include "EditorCameraController.h"

#include "Camera.h"
#include "Input.h"

#include "imgui.h"
#include "ImGuizmo.h"

void EditorCameraController::UpdateLookState(HWND window)
{
    const ImGuiIO& io = ImGui::GetIO();

    // UI 밖에서 RMB를 새로 누른 경우에만 Editor Camera Look Mode를 시작합니다.
    if (Input::WasRightMouseButtonPressed())
    {
        isLooking_ =
            !io.WantCaptureMouse && !ImGuizmo::IsUsing();
        // 누르기 전 Cursor 이동이 첫 Camera Delta에 섞이지 않도록 현재 위치를 기준점으로 삼습니다.
        Input::ResetMousePosition(window);
    }

    // RMB 해제, Focus 손실 또는 ImGui Mouse Capture 시 Look Mode를 즉시 끝냅니다.
    if (Input::WasRightMouseButtonReleased() ||
        (isLooking_ &&
            (io.WantCaptureMouse || !Input::IsRightMouseButtonDown())))
    {
        isLooking_ = false;
        Input::ResetMousePosition(window);
    }
}

void EditorCameraController::MoveCamera(Camera& camera, float deltaTime) const
{
    constexpr float cameraSpeed = 3.0f;
    constexpr float mouseSensitivity = 0.002f;

    const ImGuiIO& io = ImGui::GetIO();
    const bool isGizmoUsing = ImGuizmo::IsUsing();

    // Mouse Delta는 이미 한 Frame 동안 이동한 Pixel 양이므로 DeltaTime을 곱하지 않습니다.
    if (isLooking_ && !isGizmoUsing)
    {
        const MouseDelta mouseDelta = Input::GetMouseDelta();
        camera.Rotate(
            static_cast<float>(mouseDelta.x) * mouseSensitivity,
            static_cast<float>(-mouseDelta.y) * mouseSensitivity);
    }

    const float movementDistance = cameraSpeed * deltaTime;

    if (isLooking_ &&
        !isGizmoUsing &&
        !io.WantCaptureKeyboard)
    {
        if (Input::IsKeyDown('W'))
            camera.MoveForward(movementDistance);
        if (Input::IsKeyDown('S'))
            camera.MoveForward(-movementDistance);
        if (Input::IsKeyDown('A'))
            camera.MoveRight(-movementDistance);
        if (Input::IsKeyDown('D'))
            camera.MoveRight(movementDistance);
    }
}

bool EditorCameraController::IsLooking() const
{
    return isLooking_;
}
