#include "Input.h"

namespace
{
    POINT previousMousePosition{};
    MouseDelta mouseDelta{};
    POINT currentMousePosition{};
    bool hasPreviousMousePosition = false;
    bool hasCurrentMousePosition = false;
    bool isWindowActive = false;
    bool isLeftMouseButtonDown = false;
    bool wasLeftMouseButtonPressed = false;
    bool isRightMouseButtonDown = false;
    bool wasRightMouseButtonPressed = false;
    bool wasRightMouseButtonReleased = false;

    bool GetMouseClientPosition(HWND window, POINT& mousePosition)
    {
        return GetCursorPos(&mousePosition) &&
            ScreenToClient(window, &mousePosition);
    }
}

void Input::Update(HWND window)
{
    mouseDelta = {};
    hasCurrentMousePosition = false;
    isWindowActive = window != nullptr && GetForegroundWindow() == window;

    const bool previousLeftMouseButtonDown = isLeftMouseButtonDown;
    isLeftMouseButtonDown = isWindowActive &&
        (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    wasLeftMouseButtonPressed =
        isLeftMouseButtonDown && !previousLeftMouseButtonDown;

    const bool previousRightMouseButtonDown = isRightMouseButtonDown;
    isRightMouseButtonDown = isWindowActive &&
        (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    wasRightMouseButtonPressed =
        isRightMouseButtonDown && !previousRightMouseButtonDown;
    wasRightMouseButtonReleased =
        !isRightMouseButtonDown && previousRightMouseButtonDown;

    // 비활성화 중 Mouse 이동을 누적하지 않고, 다시 활성화된 첫 Frame도 회전시키지 않습니다.
    if (!isWindowActive)
    {
        hasPreviousMousePosition = false;
        return;
    }

    POINT mousePosition{};
    if (!GetMouseClientPosition(window, mousePosition))
    {
        hasPreviousMousePosition = false;
        return;
    }
    currentMousePosition = mousePosition;
    hasCurrentMousePosition = true;

    // 비교할 이전 위치가 없는 첫 Frame에는 현재 위치만 저장해 큰 Mouse Jump를 방지합니다.
    if (!hasPreviousMousePosition)
    {
        previousMousePosition = mousePosition;
        hasPreviousMousePosition = true;
        return;
    }

    mouseDelta.x = mousePosition.x - previousMousePosition.x;
    mouseDelta.y = mousePosition.y - previousMousePosition.y;
    previousMousePosition = mousePosition;
}

bool Input::IsKeyDown(int virtualKey)
{
    // 최상위 Bit가 1이면 호출 시점에 해당 Key가 눌려 있습니다.
    return isWindowActive && (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

bool Input::WasLeftMouseButtonPressed()
{
    return wasLeftMouseButtonPressed;
}

bool Input::IsRightMouseButtonDown()
{
    return isRightMouseButtonDown;
}

bool Input::WasRightMouseButtonPressed()
{
    return wasRightMouseButtonPressed;
}

bool Input::WasRightMouseButtonReleased()
{
    return wasRightMouseButtonReleased;
}

bool Input::GetMousePosition(POINT& mousePosition)
{
    if (!isWindowActive || !hasCurrentMousePosition)
    {
        return false;
    }

    mousePosition = currentMousePosition;
    return true;
}

void Input::ResetMousePosition(HWND window)
{
    mouseDelta = {};

    POINT currentMousePosition{};
    if (window == nullptr || GetForegroundWindow() != window ||
        !GetMouseClientPosition(window, currentMousePosition))
    {
        hasPreviousMousePosition = false;
        return;
    }

    // Look Mode가 시작되거나 끝난 현재 위치를 다음 Delta의 기준점으로 사용합니다.
    previousMousePosition = currentMousePosition;
    hasPreviousMousePosition = true;
}

MouseDelta Input::GetMouseDelta()
{
    return mouseDelta;
}
