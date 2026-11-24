#pragma once

class EngineGuide;
class GuideNavigation;
struct GuideCard;

// 카드 한 장의 그림과 설명, 이어진 카드를 보여 줍니다.
class GuideCardPage final
{
public:
    GuideCardPage() = delete;

    static void Draw(
        const GuideCard& card,
        const EngineGuide& guide,
        GuideNavigation& navigation);

private:
    static void DrawHeader(const GuideCard& card);
    static void DrawFunctions(const GuideCard& card);
    static void DrawUsingFeatures(
        const GuideCard& card,
        const EngineGuide& guide,
        GuideNavigation& navigation);
};
