#pragma once

#include <Windows.h>
#include <dxgiformat.h>

// SwapChain, BackBuffer, CommandAllocator가 함께 사용하는 Frame 개수입니다.
namespace D3D12Config
{
    constexpr UINT FrameCount = 2;

    // BackBuffer Resource의 형식이며 ImGui는 이 형식 그대로 색을 씁니다.
    constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Scene을 그릴 때 쓰는 RTV 형식입니다. Shader가 계산한 선형 색을 GPU가 sRGB로 바꿔 저장합니다.
    constexpr DXGI_FORMAT SceneTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    // 앞뒤를 비교할 깊이 값의 형식입니다.
    constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;
}
