#pragma once

class EngineGuideWindow;

// 편집기 기능 옆에, 누르면 설명이 열리는 물음표를 그립니다.
class GuideHelpButton final
{
public:
    GuideHelpButton() = delete;

    // 바로 앞에 그린 UI와 같은 줄에 이어 그립니다.
    static void Draw(EngineGuideWindow& guideWindow, const char* id);

    // 바로 앞에 그린 UI와 같은 줄을 유지하되 Panel 오른쪽 끝에 맞춰 그립니다.
    static void DrawAtRight(EngineGuideWindow& guideWindow, const char* id);

    // Panel 폭을 채우는 UI가 물음표 자리를 남겨 둘 때 사용하는 폭입니다.
    static float GetReservedWidth();

private:
    static void DrawOnCurrentLine(
        EngineGuideWindow& guideWindow,
        const char* id);
};
