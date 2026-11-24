#pragma once

#include <string>
#include <vector>

class EngineGuide;
class GuideNavigation;
struct GuideCommit;
struct GuideStage;

// 단계 하나에서 만든 것과 커밋, 그 커밋에서 생긴 클래스를 보여 줍니다.
class GuideStagePage final
{
public:
    GuideStagePage() = delete;

    static void Draw(
        const GuideStage& stage,
        const EngineGuide& guide,
        GuideNavigation& navigation);

private:
    // 이 단계에서 생긴 카드를 커밋 순서대로 모읍니다.
    static std::vector<std::string> GetCardIds(const GuideStage& stage);

    static void DrawCommit(
        const GuideCommit& commit,
        const EngineGuide& guide,
        GuideNavigation& navigation);
};
