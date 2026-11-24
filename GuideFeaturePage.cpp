#include "GuideFeaturePage.h"

#include "EngineGuide.h"
#include "GuideCardGrid.h"
#include "GuideNavigation.h"
#include "GuideTheme.h"

#include "imgui.h"

#include <algorithm>
#include <vector>

namespace
{
    constexpr float CircleRadius = 15.0f;
    constexpr float TextIndent = 46.0f;
}

void GuideFeaturePage::Draw(
    const GuideFeature& feature,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    GuideTheme::DrawHeading(feature.name.c_str(), GuideTheme::TitleFontSize);
    ImGui::TextWrapped("%s", feature.howTo.c_str());
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    GuideTheme::DrawHeading("움직이는 순서", GuideTheme::HeadingFontSize);

    std::vector<std::string> cardIds;
    for (std::size_t index = 0; index < feature.steps.size(); ++index)
    {
        const GuideStep& step = feature.steps[index];
        const GuideCard* const card = guide.FindCard(step.cardId);
        if (card == nullptr)
        {
            continue;
        }
        if (std::find(cardIds.begin(), cardIds.end(), card->id) ==
            cardIds.end())
        {
            cardIds.push_back(card->id);
        }

        ImGui::PushID(static_cast<int>(index));
        const ImVec2 start = ImGui::GetCursorScreenPos();
        ImGui::Indent(TextIndent);
        if (ImGui::Button(card->name.c_str()))
        {
            navigation.Request(card->id);
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, GuideTheme::MutedTextColor);
        ImGui::TextUnformatted(step.functionName.c_str());
        ImGui::PopStyleColor();
        ImGui::TextWrapped("%s", step.description.c_str());
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::Unindent(TextIndent);

        DrawStepMarker(
            index + 1,
            card->groupColor,
            start.x + CircleRadius + 1.0f,
            start.y + CircleRadius,
            index + 1 == feature.steps.size());
        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    GuideTheme::DrawHeading("거쳐 가는 카드", GuideTheme::HeadingFontSize);
    GuideCardGrid::Draw(cardIds, guide, navigation);
}

void GuideFeaturePage::DrawStepMarker(
    std::size_t stepNumber,
    const std::string& groupColor,
    float centerX,
    float centerY,
    bool isLast)
{
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const ImVec2 center(centerX, centerY);
    if (!isLast)
    {
        drawList->AddLine(
            ImVec2(center.x, center.y + CircleRadius),
            ImVec2(center.x, ImGui::GetCursorScreenPos().y),
            GuideTheme::LineColor,
            2.0f);
    }
    drawList->AddCircleFilled(
        center,
        CircleRadius,
        GuideTheme::GetPlateColors(groupColor).lit);
    drawList->AddCircle(center, CircleRadius, GuideTheme::LineColor, 0, 2.0f);

    const std::string number = std::to_string(stepNumber);
    const ImVec2 numberSize =
        GuideTheme::GetTextSize(GuideTheme::BodyFontSize, number.c_str());
    drawList->AddText(
        ImGui::GetFont(),
        GuideTheme::BodyFontSize,
        ImVec2(
            center.x - numberSize.x * 0.5f,
            center.y - numberSize.y * 0.5f),
        GuideTheme::TextColor,
        number.c_str());
}
