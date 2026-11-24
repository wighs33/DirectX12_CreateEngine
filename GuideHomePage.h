#pragma once

class EngineGuide;
class GuideNavigation;

// 모든 기능과 카드를 묶음별로 보여 주는 첫 화면입니다.
class GuideHomePage final
{
public:
    GuideHomePage() = delete;

    static void Draw(const EngineGuide& guide, GuideNavigation& navigation);
};
