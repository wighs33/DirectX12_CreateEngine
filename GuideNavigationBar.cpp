#include "GuideNavigationBar.h"

#include "EngineGuide.h"
#include "GuideNavigation.h"

#include "imgui.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    // 지금 보는 것의 앞과 뒤로 가는 버튼을 그립니다. 목록에 없으면 false를 반환합니다.
    template <typename T>
    bool DrawNeighbors(
        const std::vector<T>& items,
        GuideNavigation& navigation)
    {
        const auto current = std::find_if(
            items.begin(),
            items.end(),
            [&navigation](const T& item)
            {
                return item.id == navigation.GetCurrentId();
            });
        if (current == items.end())
        {
            return false;
        }

        if (current != items.begin())
        {
            const T& previous = *(current - 1);
            ImGui::SameLine();
            if (ImGui::Button(("이전: " + previous.name + "###Previous").c_str()))
            {
                navigation.Request(previous.id);
            }
        }
        if (current + 1 != items.end())
        {
            const T& next = *(current + 1);
            ImGui::SameLine();
            if (ImGui::Button(("다음: " + next.name + "###Next").c_str()))
            {
                navigation.Request(next.id);
            }
        }
        return true;
    }
}

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

    if (!DrawNeighbors(guide.GetCards(), navigation))
    {
        DrawNeighbors(guide.GetStages(), navigation);
    }
}
