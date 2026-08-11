#pragma once

#include <Windows.h>

// SwapChain, BackBuffer, CommandAllocator가 함께 사용하는 Frame 개수입니다.
namespace D3D12Config
{
    constexpr UINT FrameCount = 2;
}
