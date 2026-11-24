#include "InspectorPanel.h"

#include "EditorLayout.h"
#include "EditorSelection.h"
#include "GameObject.h"
#include "GuideHelpButton.h"
#include "Scene.h"
#include "Transform.h"

#include "imgui.h"

#include <DirectXMath.h>

void InspectorPanel::Draw(
    Scene& scene,
    EditorSelection& selection,
    EngineGuideWindow& guideWindow)
{
    EditorLayout::BeginPanel("Inspector", EditorLayout::GetInspectorPanel());
    GameObject* const selectedObject = selection.GetSelected(scene);
    if (selectedObject == nullptr)
    {
        ImGui::TextDisabled("No Object Selected");
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Name");
    ImGui::TextUnformatted(selectedObject->GetName().c_str());

    ImGui::SeparatorText("Transform");
    GuideHelpButton::DrawAtRight(guideWindow, "feature-inspector");
    DrawTransform(selectedObject->GetTransform());

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button(
            "Delete Object",
            ImVec2(-GuideHelpButton::GetReservedWidth(), 0.0f)))
    {
        selection.RequestDelete();
    }
    GuideHelpButton::DrawAtRight(guideWindow, "feature-delete");
    ImGui::End();
}

void InspectorPanel::DrawTransform(Transform& transform)
{
    const DirectX::XMFLOAT3& currentPosition = transform.GetPosition();
    float position[] =
    {
        currentPosition.x,
        currentPosition.y,
        currentPosition.z
    };
    if (ImGui::DragFloat3("Position", position, 0.1f, 0.0f, 0.0f, "%.2f"))
    {
        transform.SetPosition(position[0], position[1], position[2]);
    }

    const DirectX::XMFLOAT3& currentRotation = transform.GetRotation();
    float rotationDegrees[] =
    {
        DirectX::XMConvertToDegrees(currentRotation.x),
        DirectX::XMConvertToDegrees(currentRotation.y),
        DirectX::XMConvertToDegrees(currentRotation.z)
    };
    if (ImGui::DragFloat3(
        "Rotation",
        rotationDegrees,
        1.0f,
        0.0f,
        0.0f,
        "%.1f"))
    {
        transform.SetRotation(
            DirectX::XMConvertToRadians(rotationDegrees[0]),
            DirectX::XMConvertToRadians(rotationDegrees[1]),
            DirectX::XMConvertToRadians(rotationDegrees[2]));
    }

    const DirectX::XMFLOAT3& currentScale = transform.GetScale();
    float scale[] =
    {
        currentScale.x,
        currentScale.y,
        currentScale.z
    };
    if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.0f, 0.0f, "%.2f"))
    {
        transform.SetScale(scale[0], scale[1], scale[2]);
    }
}
