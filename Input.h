#pragma once

#include <Windows.h>

// 이전 Frame과 현재 Frame 사이에서 Mouse가 움직인 Client 좌표 차이입니다.
struct MouseDelta
{
    LONG x = 0;
    LONG y = 0;
};

// 현재 Frame에 필요한 Keyboard 상태와 Mouse 이동량을 Polling 방식으로 확인합니다.
class Input final
{
public:
    Input() = delete;

    static void Update(HWND window);
    static bool IsKeyDown(int virtualKey);
    static bool WasLeftMouseButtonPressed();
    static bool IsRightMouseButtonDown();
    static bool WasRightMouseButtonPressed();
    static bool WasRightMouseButtonReleased();
    static bool GetMousePosition(POINT& mousePosition);
    static void ResetMousePosition(HWND window);
    static MouseDelta GetMouseDelta();
};
