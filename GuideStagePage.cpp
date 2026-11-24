#include "GuideStagePage.h"

#include "EngineGuide.h"
#include "GuideCardGrid.h"
#include "GuideLinkButtons.h"
#include "GuideTheme.h"

#include "imgui.h"

namespace
{
    // 커밋 번호 뒤에 제목이 시작되는 자리입니다.
    constexpr float TitleOffset = 92.0f;
}

void GuideStagePage::Draw(
    const GuideStage& stage,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    const std::vector<std::string> cardIds = GetCardIds(stage);

    GuideTheme::DrawHeading(stage.name.c_str(), GuideTheme::TitleFontSize);
    GuideTheme::DrawMutedText(
        stage.date + " · 커밋 " + std::to_string(stage.commits.size()) +
        "개 · 카드 " + std::to_string(cardIds.size()) + "장");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    GuideTheme::DrawSection("만든 것", stage.made);
    GuideTheme::DrawSection("새로 배운 것", stage.learned);
    GuideTheme::DrawSection("확인한 것", stage.checked);

    GuideTheme::DrawHeading("커밋과 클래스", GuideTheme::HeadingFontSize);
    for (const GuideCommit& commit : stage.commits)
    {
        DrawCommit(commit, guide, navigation);
    }

    if (!cardIds.empty())
    {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        GuideTheme::DrawHeading(
            "이 단계에서 생긴 카드",
            GuideTheme::HeadingFontSize);
        GuideCardGrid::Draw(cardIds, guide, navigation);
    }
}

std::vector<std::string> GuideStagePage::GetCardIds(const GuideStage& stage)
{
    std::vector<std::string> cardIds;
    for (const GuideCommit& commit : stage.commits)
    {
        cardIds.insert(
            cardIds.end(),
            commit.cardIds.begin(),
            commit.cardIds.end());
    }
    return cardIds;
}

void GuideStagePage::DrawCommit(
    const GuideCommit& commit,
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    ImGui::PushStyleColor(ImGuiCol_Text, GuideTheme::MutedTextColor);
    ImGui::TextUnformatted(commit.hash.c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine(TitleOffset);
    ImGui::TextWrapped("%s", commit.title.c_str());

    std::vector<GuideLink> links;
    for (const std::string& cardId : commit.cardIds)
    {
        if (const GuideCard* const card = guide.FindCard(cardId))
        {
            links.push_back({ card->name, card->id });
        }
    }
    if (!links.empty())
    {
        ImGui::Indent(TitleOffset);
        GuideLinkButtons::Draw(links, navigation);
        ImGui::Unindent(TitleOffset);
    }
}
