#pragma once

#include <Windows.h>

// Windows 창 하나를 만들고 창으로 들어오는 메시지를 처리합니다.
class Win32Window final
{
public:
    Win32Window() = default;
    ~Win32Window();

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    // 창 내부 영역이 정확히 width x height가 되도록 만듭니다.
    bool Create(const wchar_t* title, UINT width, UINT height);
    void Show();

    // 쌓인 메시지를 모두 처리하고, 창이 닫혀 끝내야 하면 false를 반환합니다.
    bool ProcessMessages();

    HWND GetHandle() const;
    int GetExitCode() const;

private:
    // 메시지란? : WM_KEYDOWN, WM_MOUSEMOVE, WM_SIZE, WM_MOVE, WM_PAINT 등등
    static LRESULT CALLBACK WindowProc(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    HWND handle_ = nullptr;
    int exitCode_ = 0;
};
