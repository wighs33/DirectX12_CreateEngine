#include "Win32Window.h"

#include "ImGuiLayer.h"

namespace
{
    constexpr wchar_t ClassName[] = L"DirectX12StudyWindowClass";
}

Win32Window::~Win32Window()
{
    // 사용자가 닫은 창은 이미 파괴되어 있습니다.
    if (handle_ != nullptr && IsWindow(handle_))
    {
        DestroyWindow(handle_);
    }
}

bool Win32Window::Create(const wchar_t* title, UINT width, UINT height)
{
    const HINSTANCE instance = GetModuleHandleW(nullptr);

    // [1단계] WindowProc 등 창의 공통 특성을 정의하고 Windows에 등록합니다.
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = ClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (RegisterClassW(&windowClass) == 0)
    {
        return false;
    }

    // [2단계] 창 내부 영역이 요청한 크기가 되도록 바깥 크기를 보정합니다.
    constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    RECT windowRect{
        0,
        0,
        static_cast<LONG>(width),
        static_cast<LONG>(height) };
    if (!AdjustWindowRect(&windowRect, windowStyle, FALSE))
    {
        return false;
    }

    // [3단계] 등록한 Window Class와 보정한 크기로 실제 HWND를 생성합니다.
    handle_ = CreateWindowExW(
        0,
        ClassName,
        title,
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);
    return handle_ != nullptr;
}

void Win32Window::Show()
{
    ShowWindow(handle_, SW_SHOW);
}

bool Win32Window::ProcessMessages()
{
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            exitCode_ = static_cast<int>(message.wParam);
            return false;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return true;
}

HWND Win32Window::GetHandle() const
{
    return handle_;
}

int Win32Window::GetExitCode() const
{
    return exitCode_;
}

// 창 생성 중 또는 DispatchMessageW가 전달한 메시지 하나를 처리합니다.
LRESULT CALLBACK Win32Window::WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    // 모든 Win32 입력을 ImGui에 먼저 전달하고, 처리한 메시지만 기존 Application 경로에서 제외합니다.
    if (ImGuiLayer::HandleMessage(window, message, wParam, lParam))
    {
        return 1;
    }

    if (message == WM_DESTROY)
    {
        // 창이 파괴되면 메시지 루프가 끝나도록 WM_QUIT 메시지를 보냅니다.
        PostQuitMessage(0);
        return 0;
    }

    // 직접 처리하지 않은 메시지는 Windows의 기본 처리에 맡깁니다.
    return DefWindowProcW(window, message, wParam, lParam);
}
