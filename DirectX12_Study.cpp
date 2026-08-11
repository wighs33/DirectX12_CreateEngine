#include <Windows.h>

#include "D3D12Context.h"
#include "Timer.h"

// 창 생성 중 또는 DispatchMessageW가 전달한 메시지 하나를 처리합니다.
// 메시지란? : WM_KEYDOWN, WM_MOUSEMOVE, WM_SIZE, WM_MOVE, WM_PAINT 등등
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        // 창이 파괴되면 메시지 루프가 끝나도록 WM_QUIT 메시지를 보냅니다.
        PostQuitMessage(0);
        return 0;
    }

    // 직접 처리하지 않은 메시지는 Windows의 기본 처리에 맡깁니다.
    return DefWindowProcW(window, message, wParam, lParam);
}

// 전체 흐름 : Win32 창 생성 -> DirectX 12 초기화 -> Message/Render Loop -> 종료
int main()
{
    constexpr UINT clientWidth = 1280;
    constexpr UINT clientHeight = 720;

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"DirectX12StudyWindowClass";

    // [1단계] WindowProc 등 창의 공통 특성을 정의하고 Windows에 등록합니다.
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (RegisterClassW(&windowClass) == 0)
    {
        return 1;
    }

    constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    RECT windowRect{0, 0, clientWidth, clientHeight};

    // [2단계] 창 내부 영역이 정확히 1280x720이 되도록 바깥 크기를 보정합니다.
    if (!AdjustWindowRect(&windowRect, windowStyle, FALSE))
    {
        return 1;
    }

    // [3단계] 등록한 Window Class와 보정한 크기로 실제 HWND를 생성합니다.
    const HWND window = CreateWindowExW(
        0,
        className,
        L"DirectX 12 Study",
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (window == nullptr)
    {
        return 1;
    }

    // [4단계] HWND에 필요한 DirectX 12 객체를 D3D12Context 내부에서 순서대로 생성합니다.
    D3D12Context d3d12Context;
    if (!d3d12Context.Initialize(window, clientWidth, clientHeight))
    {
        return 1;
    }

    ShowWindow(window, SW_SHOW);

    Timer timer;

    // [5단계] Message 처리 후 Tick -> Update -> Render 순서로 한 Frame을 실행합니다.
    MSG message{};
    bool isRunning = true;
    while (isRunning)
    {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                isRunning = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (isRunning)
        {
            timer.Tick();
            d3d12Context.Update(timer.GetDeltaTime());

            if (!d3d12Context.RenderFrame())
            {
                return 1;
            }
        }
    }

    // [6단계] main이 끝나면 D3D12Context가 DirectX 객체와 Event Handle을 정리합니다.
    return static_cast<int>(message.wParam);
}
