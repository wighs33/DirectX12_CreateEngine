#include "Input.h"

namespace
{
    POINT previousMousePosition{};
    MouseDelta mouseDelta{};
    bool hasPreviousMousePosition = false;
    bool isWindowActive = false;
}

void Input::Update(HWND window)
{
    mouseDelta = {};
    isWindowActive = window != nullptr && GetForegroundWindow() == window;

    // 비활성화 중 Mouse 이동을 누적하지 않고, 다시 활성화된 첫 Frame도 회전시키지 않습니다.
    if (!isWindowActive)
    {
        hasPreviousMousePosition = false;
        return;
    }

    POINT currentMousePosition{};
    if (!GetCursorPos(&currentMousePosition) ||
        !ScreenToClient(window, &currentMousePosition))
    {
        hasPreviousMousePosition = false;
        return;
    }

    // 비교할 이전 위치가 없는 첫 Frame에는 현재 위치만 저장해 큰 Mouse Jump를 방지합니다.
    if (!hasPreviousMousePosition)
    {
        previousMousePosition = currentMousePosition;
        hasPreviousMousePosition = true;
        return;
    }

    mouseDelta.x = currentMousePosition.x - previousMousePosition.x;
    mouseDelta.y = currentMousePosition.y - previousMousePosition.y;
    previousMousePosition = currentMousePosition;
}

bool Input::IsKeyDown(int virtualKey)
{
    // 최상위 Bit가 1이면 호출 시점에 해당 Key가 눌려 있습니다.
    return isWindowActive && (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

MouseDelta Input::GetMouseDelta()
{
    return mouseDelta;
}
