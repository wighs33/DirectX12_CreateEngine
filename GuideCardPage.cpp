#include "GuideCardPage.h"

#include "EngineGuide.h"
#include "GuideCardGrid.h"
#include "GuideLinkButtons.h"
#include "GuidePictureRenderer.h"
#include "GuideTheme.h"

#include "imgui.h"

#include <algorithm>
#include <vector>

void GuideCardPage::Draw(
    const GuideCard& card,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    DrawHeader(card);

    GuideTheme::DrawSection("없으면 어떻게 될까", card.without);
    GuideTheme::DrawSection("지금의 한계", card.limit);
    GuideTheme::DrawSection("언리얼에서는", card.unreal);

    DrawFunctions(card);
    DrawUsingFeatures(card, guide, navigation);
    DrawOrigin(card, guide, navigation);

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    GuideTheme::DrawHeading("이어진 카드", GuideTheme::HeadingFontSize);
    GuideCardGrid::Draw(card.links, guide, navigation);
}

void GuideCardPage::DrawHeader(const GuideCard& card)
{
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float pictureWidth = (std::min)(440.0f, availableWidth * 0.5f);
    const ImVec2 picturePosition = ImGui::GetCursorScreenPos();
    GuidePictureRenderer::Draw(
        card,
        { picturePosition.x, picturePosition.y },
        pictureWidth);
    ImGui::Dummy(ImVec2(
        pictureWidth,
        GuidePictureRenderer::GetHeight(pictureWidth)));

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::BeginGroup();
    GuideTheme::DrawHeading(card.name.c_str(), GuideTheme::TitleFontSize);
    ImGui::TextWrapped("%s", card.role.c_str());
    GuideTheme::DrawMutedText(card.groupName + " · " + card.file);
    ImGui::EndGroup();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

void GuideCardPage::DrawFunctions(const GuideCard& card)
{
    GuideTheme::DrawHeading("하는 일", GuideTheme::HeadingFontSize);
    if (ImGui::BeginTable(
            "Functions",
            2,
            ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("함수나 코드", 0, 0.38f);
        ImGui::TableSetupColumn("설명", 0, 0.62f);
        ImGui::TableHeadersRow();
        for (const GuideFunction& function : card.functions)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", function.name.c_str());
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", function.description.c_str());
        }
        ImGui::EndTable();
    }
}

void GuideCardPage::DrawUsingFeatures(
    const GuideCard& card,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    std::vector<GuideLink> links;
    for (const GuideFeature& feature : guide.GetFeatures())
    {
        const bool usesCard = std::any_of(
            feature.steps.begin(),
            feature.steps.end(),
            [&card](const GuideStep& step) { return step.cardId == card.id; });
        if (usesCard)
        {
            links.push_back({ "?  " + feature.name, feature.id });
        }
    }
    if (!links.empty())
    {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        GuideTheme::DrawHeading("이 카드를 쓰는 기능", GuideTheme::HeadingFontSize);
        GuideLinkButtons::Draw(links, navigation);
    }
}

void GuideCardPage::DrawOrigin(
    const GuideCard& card,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    for (const GuideStage& stage : guide.GetStages())
    {
        for (const GuideCommit& commit : stage.commits)
        {
            if (std::find(
                    commit.cardIds.begin(),
                    commit.cardIds.end(),
                    card.id) == commit.cardIds.end())
            {
                continue;
            }

            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            GuideTheme::DrawHeading("생긴 단계", GuideTheme::HeadingFontSize);
            GuideTheme::DrawMutedText(commit.hash + " · " + commit.title);
            GuideLinkButtons::Draw({ { stage.name, stage.id } }, navigation);
            return;
        }
    }
}
