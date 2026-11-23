#pragma once

class EngineGuideWindow;

// 패널이 없는 기능의 물음표와 도감 버튼을 화면 아래에 모아 둡니다.
class ViewportHelpBar final
{
public:
    ViewportHelpBar() = delete;

    static void Draw(EngineGuideWindow& guideWindow);

private:
    static void DrawItem(
        const char* label,
        const char* guideId,
        EngineGuideWindow& guideWindow);
};
