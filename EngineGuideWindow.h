#pragma once

#include "EngineGuide.h"
#include "GuideNavigation.h"
#include "GuideTheme.h"

#include <filesystem>
#include <string>
#include <vector>

// Editor 기능 옆의 물음표와, 누르면 열리는 엔진 도감 창을 ImGui로 그립니다.
class EngineGuideWindow final
{
public:
    // ImGui Context를 만든 뒤 첫 Frame 전에 한 번 호출합니다.
    // 도감 파일을 읽지 못해도 Editor는 계속 사용할 수 있으며 창에 안내만 표시합니다.
    void Initialize(const std::filesystem::path& guideFilePath);

    // 바로 앞에 그린 UI와 같은 줄에 물음표를 이어 그립니다.
    void DrawHelpButton(const char* id);

    // 바로 앞에 그린 UI와 같은 줄을 유지하되 Panel 오른쪽 끝에 맞춰 물음표를 그립니다.
    void DrawHelpButtonAtRight(const char* id);

    // Panel 폭을 채우는 UI가 물음표 자리를 남겨 둘 때 사용하는 폭입니다.
    static float GetHelpButtonReservedWidth();

    void OpenHome();

    // 카드 또는 기능의 이름에 해당하는 설명으로 창을 엽니다.
    void Open(const std::string& id);

    // 창이 열려 있을 때 현재 화면을 그립니다.
    void Draw();

private:
    void DrawHelpButtonOnCurrentLine(const char* id);

    void DrawNavigationBar();
    void DrawHome();
    void DrawCardPage(const GuideCard& card);
    void DrawFeaturePage(const GuideFeature& feature);
    void DrawFeatureButtons(const std::vector<const GuideFeature*>& features);
    void DrawCardGrid(const std::vector<std::string>& cardIds);
    void DrawCardButton(const GuideCard& card, float width, float height);

    EngineGuide guide_;
    GuideTheme theme_;
    GuideNavigation navigation_;
    bool isLoaded_ = false;

    bool isOpen_ = false;
    bool isFocusRequested_ = false;
    bool isScrollResetRequested_ = false;
};
