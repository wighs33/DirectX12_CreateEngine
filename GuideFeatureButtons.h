#pragma once

#include <vector>

class GuideNavigation;
struct GuideFeature;

// 기능의 이름을 버튼으로 늘어놓고, 누르면 그 설명으로 갑니다.
class GuideFeatureButtons final
{
public:
    GuideFeatureButtons() = delete;

    // 한 줄에 들어가지 않으면 다음 줄로 넘깁니다.
    static void Draw(
        const std::vector<const GuideFeature*>& features,
        GuideNavigation& navigation);
};
