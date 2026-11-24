#pragma once

#include <string>
#include <vector>

class GuideNavigation;

// 버튼 하나의 글자와, 눌렀을 때 갈 설명의 이름입니다.
struct GuideLink
{
    std::string label;
    std::string id;
};

// 갈 곳을 버튼으로 늘어놓고, 누르면 그 설명으로 갑니다.
class GuideLinkButtons final
{
public:
    GuideLinkButtons() = delete;

    // 한 줄에 들어가지 않으면 다음 줄로 넘깁니다.
    static void Draw(
        const std::vector<GuideLink>& links,
        GuideNavigation& navigation);
};
