#pragma once

#include <string>
#include <vector>

// 도감에서 지금 보는 화면과 지나온 화면을 기억합니다.
// 그리는 도중에 화면이 바뀌지 않도록 이동은 Apply에서 한꺼번에 처리합니다.
class GuideNavigation final
{
public:
    // 빈 문자열은 전체 카드가 보이는 첫 화면입니다.
    void Request(const std::string& id);
    void RequestBack();

    // 요청을 처리하고 화면이 바뀌었으면 true를 반환합니다.
    bool Apply();

    const std::string& GetCurrentId() const;
    bool CanGoBack() const;

private:
    std::string currentId_;
    std::vector<std::string> history_;
    std::string requestedId_;
    bool hasRequest_ = false;
    bool isBackRequested_ = false;
};
