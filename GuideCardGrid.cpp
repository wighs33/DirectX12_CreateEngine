#include "GuideCardGrid.h"

#include "EngineGuide.h"
#include "GuideNavigation.h"
#include "GuidePictureRenderer.h"
#include "GuideTheme.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

void GuideCardGrid::Draw(
    const std::vector<std::string>& cardIds,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    constexpr float minimumCardWidth = 270.0f;
    constexpr float cardSpacing = 12.0f;

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const int columnCount = (std::max)(
        1,
        static_cast<int>(
            (availableWidth + cardSpacing) / (minimumCardWidth + cardSpacing)));
    const float cardWidth = std::floor(
        (availableWidth - cardSpacing * static_cast<float>(columnCount - 1)) /
        static_cast<float>(columnCount));

    // 같은 묶음의 카드 높이를 맞추기 위해 가장 긴 설명을 기준으로 삼습니다.
    float roleHeight = 0.0f;
    for (const std::string& cardId : cardIds)
    {
        if (const GuideCard* const card = guide.FindCard(cardId))
        {
            roleHeight = (std::max)(
                roleHeight,
                GuideTheme::GetTextSize(
                    GuideTheme::SmallFontSize,
                    card->role.c_str(),
                    cardWidth - 28.0f).y);
        }
    }
    const float cardHeight =
        GuidePictureRenderer::GetHeight(cardWidth) +
        GuideTheme::BodyFontSize + roleHeight + 30.0f;

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(cardSpacing, cardSpacing));
    int column = 0;
    for (const std::string& cardId : cardIds)
    {
        const GuideCard* const card = guide.FindCard(cardId);
        if (card == nullptr)
        {
            continue;
        }

        if (column > 0)
        {
            ImGui::SameLine();
        }
        DrawCard(*card, cardWidth, cardHeight, navigation);
        column = (column + 1) % columnCount;
    }
    ImGui::PopStyleVar();
}

void GuideCardGrid::DrawCard(
    const GuideCard& card,
    float width,
    float height,
    GuideNavigation& navigation)
{
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    const ImVec2 maximum(minimum.x + width, minimum.y + height);

    ImGui::PushID(card.id.c_str());
    if (ImGui::InvisibleButton("Card", ImVec2(width, height)))
    {
        navigation.Request(card.id);
    }
    const bool isHovered = ImGui::IsItemHovered();
    ImGui::PopID();
    if (isHovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(minimum, maximum, GuideTheme::PaperColor);
    GuidePictureRenderer::Draw(card, { minimum.x, minimum.y }, width);

    const float pictureBottom =
        minimum.y + GuidePictureRenderer::GetHeight(width);
    drawList->AddLine(
        ImVec2(minimum.x, pictureBottom),
        ImVec2(maximum.x, pictureBottom),
        GuideTheme::LineColor,
        2.0f);
    drawList->AddText(
        ImGui::GetFont(),
        GuideTheme::BodyFontSize,
        ImVec2(minimum.x + 14.0f, pictureBottom + 9.0f),
        GuideTheme::TextColor,
        card.name.c_str());
    drawList->AddText(
        ImGui::GetFont(),
        GuideTheme::SmallFontSize,
        ImVec2(minimum.x + 14.0f, pictureBottom + GuideTheme::BodyFontSize + 15.0f),
        GuideTheme::MutedTextColor,
        card.role.c_str(),
        nullptr,
        width - 28.0f);
    drawList->AddRect(
        minimum,
        maximum,
        isHovered ? GuideTheme::FlameColor : GuideTheme::LineColor,
        3.0f,
        2.0f);
}
