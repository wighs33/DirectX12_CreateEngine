#include "TransformGizmo.h"

#include "Camera.h"
#include "EditorLayout.h"
#include "EngineGuideWindow.h"
#include "GameObject.h"
#include "Input.h"

#include "imgui.h"
#include "ImGuizmo.h"

void TransformGizmo::ProcessHotkeys(bool isCameraLooking)
{
    const ImGuiIO& io = ImGui::GetIO();
    if (isCameraLooking ||
        Input::IsRightMouseButtonDown() ||
        io.WantCaptureKeyboard)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_W, false))
    {
        operation_ = Operation::Translate;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_E, false))
    {
        operation_ = Operation::Rotate;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_R, false))
    {
        operation_ = Operation::Scale;
    }
}

void TransformGizmo::DrawToolbar(EngineGuideWindow& guideWindow)
{
    EditorLayout::BeginPanelWithoutTitle(
        "Gizmo Toolbar",
        EditorLayout::GetGizmoToolbar());
    if (ImGui::RadioButton("Move (W)", operation_ == Operation::Translate))
    {
        operation_ = Operation::Translate;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate (E)", operation_ == Operation::Rotate))
    {
        operation_ = Operation::Rotate;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale (R)", operation_ == Operation::Scale))
    {
        operation_ = Operation::Scale;
    }
    guideWindow.DrawHelpButton("feature-gizmo");
    ImGui::End();
}

void TransformGizmo::Draw(
    GameObject* selectedObject,
    const Camera& camera,
    const D3D12_VIEWPORT& viewport,
    bool isCameraLooking)
{
    if (selectedObject == nullptr ||
        viewport.Width <= 0.0f ||
        viewport.Height <= 0.0f)
    {
        ImGuizmo::Enable(false);
        return;
    }

    // RMB Camera Mode에서는 Gizmo를 표시하되 입력은 받지 않도록 합니다.
    ImGuizmo::Enable(
        !isCameraLooking && !Input::IsRightMouseButtonDown());
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(
        viewport.TopLeftX,
        viewport.TopLeftY,
        viewport.Width,
        viewport.Height);

    Transform& transform = selectedObject->GetTransform();

    DirectX::XMFLOAT4X4 viewMatrix{};
    DirectX::XMFLOAT4X4 projectionMatrix{};
    DirectX::XMFLOAT4X4 objectMatrix{};
    DirectX::XMStoreFloat4x4(&viewMatrix, camera.GetViewMatrix());
    DirectX::XMStoreFloat4x4(&projectionMatrix, camera.GetProjectionMatrix());
    DirectX::XMStoreFloat4x4(&objectMatrix, transform.GetWorldMatrix());

    // DirectXMath의 CPU 행 벡터 Matrix 원본은 ImGuizmo의 메모리 배치와 직접 대응합니다.
    // GPU Constant Buffer용으로 전치한 Matrix는 이 경로에서 사용하지 않습니다.
    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    switch (operation_)
    {
    case Operation::Translate:
        operation = ImGuizmo::TRANSLATE;
        break;
    case Operation::Rotate:
        operation = ImGuizmo::ROTATE;
        break;
    case Operation::Scale:
        operation = ImGuizmo::SCALE;
        break;
    }

    // Translate/Rotate는 World, Scale은 ImGuizmo 특성에 맞춰 Local Mode를 사용합니다.
    const ImGuizmo::MODE mode = operation == ImGuizmo::SCALE
        ? ImGuizmo::LOCAL
        : ImGuizmo::WORLD;
    const bool wasModified = ImGuizmo::Manipulate(
        &viewMatrix.m[0][0],
        &projectionMatrix.m[0][0],
        operation,
        mode,
        &objectMatrix.m[0][0]);
    if (!wasModified)
    {
        return;
    }

    float translation[3]{};
    float rotationDegrees[3]{};
    float scale[3]{};
    ImGuizmo::DecomposeMatrixToComponents(
        &objectMatrix.m[0][0],
        translation,
        rotationDegrees,
        scale);

    transform.SetPosition(
        translation[0],
        translation[1],
        translation[2]);
    transform.SetRotation(
        DirectX::XMConvertToRadians(rotationDegrees[0]),
        DirectX::XMConvertToRadians(rotationDegrees[1]),
        DirectX::XMConvertToRadians(rotationDegrees[2]));
    transform.SetScale(scale[0], scale[1], scale[2]);
}

bool TransformGizmo::IsInteracting()
{
    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}
