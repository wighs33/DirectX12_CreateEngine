#pragma once

#include "EngineGuide.h"
#include "GuideNavigation.h"
#include "GuideTheme.h"

#include <filesystem>
#include <string>

// 엔진 도감 창을 여닫고 지금 볼 화면을 골라 그립니다.
class EngineGuideWindow final
{
public:
    // ImGui Context를 만든 뒤 첫 Frame 전에 한 번 호출합니다.
    // 도감 파일을 읽지 못해도 Editor는 계속 사용할 수 있으며 창에 안내만 표시합니다.
    void Initialize(const std::filesystem::path& guideFilePath);

    void OpenHome();

    // 카드, 기능, 단계의 이름에 해당하는 설명으로 창을 엽니다.
    void Open(const std::string& id);

    // 창이 열려 있을 때 현재 화면을 그립니다.
    void Draw();

private:
    // 카드, 기능, 단계, 첫 화면 중에서 지금 볼 것을 그립니다.
    void DrawPage();

    EngineGuide guide_;
    GuideTheme theme_;
    GuideNavigation navigation_;
    bool isLoaded_ = false;

    bool isOpen_ = false;
    bool isFocusRequested_ = false;
    bool isScrollResetRequested_ = false;
};
