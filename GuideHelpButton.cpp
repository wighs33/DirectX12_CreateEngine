#include "GuideHelpButton.h"

#include "EngineGuideWindow.h"

#include "imgui.h"

void GuideHelpButton::Draw(EngineGuideWindow& guideWindow, const char* id)
{
    ImGui::SameLine();
    DrawOnCurrentLine(guideWindow, id);
}

void GuideHelpButton::DrawAtRight(
    EngineGuideWindow& guideWindow,
    const char* id)
{
    // 남은 폭을 기준으로 계산하므로 Scrollbar가 생겨도 물음표가 가려지지 않습니다.
    ImGui::SameLine();
    const float buttonWidth =
        GetReservedWidth() - ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        ImGui::GetContentRegionAvail().x -
        buttonWidth);
    DrawOnCurrentLine(guideWindow, id);
}

float GuideHelpButton::GetReservedWidth()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    return ImGui::CalcTextSize("?").x +
        style.FramePadding.x * 2.0f +
        style.ItemSpacing.x;
}

void GuideHelpButton::DrawOnCurrentLine(
    EngineGuideWindow& guideWindow,
    const char* id)
{
    // 글자가 모두 '?'로 같으므로 설명 이름을 ID로 사용해 Button을 구분합니다.
    ImGui::PushID(id);
    if (ImGui::SmallButton("?"))
    {
        guideWindow.Open(id);
    }
    ImGui::PopID();
}
