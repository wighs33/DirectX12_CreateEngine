#pragma once

#include <cstddef>
#include <string>

class EngineGuide;
class GuideNavigation;
struct GuideFeature;

// 기능 하나가 어떤 카드를 거쳐 움직이는지 순서대로 보여 줍니다.
class GuideFeaturePage final
{
public:
    GuideFeaturePage() = delete;

    static void Draw(
        const GuideFeature& feature,
        const EngineGuide& guide,
        GuideNavigation& navigation);

private:
    // 순서의 번호와 다음 순서로 이어지는 선을 그립니다.
    static void DrawStepMarker(
        std::size_t stepNumber,
        const std::string& groupColor,
        float centerX,
        float centerY,
        bool isLast);
};
