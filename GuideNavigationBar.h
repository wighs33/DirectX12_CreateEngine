#pragma once

class EngineGuide;
class GuideNavigation;

// 뒤로, 전체 카드, 이전과 다음 카드나 단계로 가는 버튼을 그립니다.
class GuideNavigationBar final
{
public:
    GuideNavigationBar() = delete;

    static void Draw(const EngineGuide& guide, GuideNavigation& navigation);
};
