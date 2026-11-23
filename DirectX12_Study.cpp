#include "D3D12Context.h"
#include "Timer.h"
#include "Win32Window.h"

// 전체 흐름 : Win32 창 생성 -> DirectX 12 초기화 -> Message/Render Loop -> 종료
int main()
{
    constexpr UINT clientWidth = 1280;
    constexpr UINT clientHeight = 720;

    // [1~3단계] 창을 등록하고 1280x720 크기로 만듭니다.
    Win32Window window;
    if (!window.Create(L"DirectX 12 Study", clientWidth, clientHeight))
    {
        return 1;
    }

    // [4단계] HWND에 필요한 DirectX 12 객체를 D3D12Context 내부에서 순서대로 생성합니다.
    D3D12Context d3d12Context;
    if (!d3d12Context.Initialize(window.GetHandle(), clientWidth, clientHeight))
    {
        return 1;
    }

    window.Show();

    // [5단계] Message 처리 후 Tick -> Update -> Render 순서로 한 Frame을 실행합니다.
    Timer timer;
    while (window.ProcessMessages())
    {
        timer.Tick();
        d3d12Context.Update(timer.GetDeltaTime());

        if (!d3d12Context.RenderFrame())
        {
            return 1;
        }
    }

    // [6단계] main이 끝나면 D3D12Context가 DirectX 객체와 Event Handle을 정리합니다.
    return window.GetExitCode();
}
