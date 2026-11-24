#include "GuideFeatureButtons.h"

#include "EngineGuideData.h"
#include "GuideNavigation.h"

#include "imgui.h"

#include <cstddef>
#include <string>

void GuideFeatureButtons::Draw(
    const std::vector<const GuideFeature*>& features,
    GuideNavigation& navigation)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float rightEdge =
        ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

    for (std::size_t index = 0; index < features.size(); ++index)
    {
        const std::string label = "?  " + features[index]->name;
        const float width =
            ImGui::CalcTextSize(label.c_str()).x +
            style.FramePadding.x * 2.0f;

        // 앞 Button 옆에 들어갈 자리가 있을 때만 같은 줄에 이어 그립니다.
        if (index > 0 &&
            ImGui::GetItemRectMax().x + style.ItemSpacing.x + width <= rightEdge)
        {
            ImGui::SameLine();
        }
        if (ImGui::Button(label.c_str()))
        {
            navigation.Request(features[index]->id);
        }
    }
}
