#pragma once

#include <string>
#include <vector>

class EngineGuide;
class GuideNavigation;
struct GuideCard;

// 카드를 격자로 늘어놓고, 누르면 그 카드로 갑니다.
class GuideCardGrid final
{
public:
    GuideCardGrid() = delete;

    // 같은 줄의 카드 높이는 가장 긴 설명에 맞춥니다.
    static void Draw(
        const std::vector<std::string>& cardIds,
        const EngineGuide& guide,
        GuideNavigation& navigation);

private:
    static void DrawCard(
        const GuideCard& card,
        float width,
        float height,
        GuideNavigation& navigation);
};
