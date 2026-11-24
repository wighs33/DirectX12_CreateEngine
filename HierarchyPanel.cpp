#include "HierarchyPanel.h"

#include "EditorLayout.h"
#include "EditorSelection.h"
#include "GameObject.h"
#include "GuideHelpButton.h"
#include "Scene.h"

#include "imgui.h"

#include <cstddef>
#include <vector>

void HierarchyPanel::Draw(
    const Scene& scene,
    EditorSelection& selection,
    EngineGuideWindow& guideWindow)
{
    EditorLayout::BeginPanel("Hierarchy", EditorLayout::GetHierarchyPanel());
    ImGui::TextUnformatted("Objects");
    GuideHelpButton::DrawAtRight(guideWindow, "feature-hierarchy");
    ImGui::Separator();

    const std::vector<GameObject>& gameObjects = scene.GetGameObjects();
    if (gameObjects.empty())
    {
        ImGui::TextDisabled("No Objects");
    }
    for (std::size_t index = 0; index < gameObjects.size(); ++index)
    {
        // 같은 이름의 물체가 있어도 구분되도록 번호를 ID로 씁니다.
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::Selectable(
                gameObjects[index].GetName().c_str(),
                selection.IsSelected(index)))
        {
            selection.Select(index);
        }
        ImGui::PopID();
    }
    ImGui::End();
}
