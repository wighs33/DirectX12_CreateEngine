#include "GuideNavigationBar.h"

#include "EngineGuide.h"
#include "GuideNavigation.h"

#include "imgui.h"

#include <algorithm>
#include <string>
#include <vector>

void GuideNavigationBar::Draw(
    const EngineGuide& guide,
    GuideNavigation& navigation)
{
    ImGui::BeginDisabled(!navigation.CanGoBack());
    if (ImGui::Button("← 뒤로"))
    {
        navigation.RequestBack();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(navigation.GetCurrentId().empty());
    if (ImGui::Button("전체 카드"))
    {
        navigation.Request(std::string());
    }
    ImGui::EndDisabled();

    const std::vector<GuideCard>& cards = guide.GetCards();
    const auto current = std::find_if(
        cards.begin(),
        cards.end(),
        [&navigation](const GuideCard& card)
        {
            return card.id == navigation.GetCurrentId();
        });
    if (current == cards.end())
    {
        return;
    }

    if (current != cards.begin())
    {
        const GuideCard& previous = *(current - 1);
        ImGui::SameLine();
        if (ImGui::Button(("이전: " + previous.name + "###Previous").c_str()))
        {
            navigation.Request(previous.id);
        }
    }
    if (current + 1 != cards.end())
    {
        const GuideCard& next = *(current + 1);
        ImGui::SameLine();
        if (ImGui::Button(("다음: " + next.name + "###Next").c_str()))
        {
            navigation.Request(next.id);
        }
    }
}
