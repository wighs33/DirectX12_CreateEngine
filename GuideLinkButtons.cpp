#include "GuideLinkButtons.h"

#include "GuideNavigation.h"

#include "imgui.h"

#include <cstddef>

void GuideLinkButtons::Draw(
    const std::vector<GuideLink>& links,
    GuideNavigation& navigation)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float rightEdge =
        ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

    for (std::size_t index = 0; index < links.size(); ++index)
    {
        const GuideLink& link = links[index];
        const float width =
            ImGui::CalcTextSize(link.label.c_str()).x +
            style.FramePadding.x * 2.0f;

        // 앞 Button 옆에 들어갈 자리가 있을 때만 같은 줄에 이어 그립니다.
        if (index > 0 &&
            ImGui::GetItemRectMax().x + style.ItemSpacing.x + width <= rightEdge)
        {
            ImGui::SameLine();
        }

        ImGui::PushID(link.id.c_str());
        if (ImGui::Button(link.label.c_str()))
        {
            navigation.Request(link.id);
        }
        ImGui::PopID();
    }
}
