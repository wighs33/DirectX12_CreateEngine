#pragma once

#include <d3d12.h>

// 자원의 쓰임이 바뀐다고 GPU에 알립니다. 앞선 쓰기가 끝난 뒤에 다음 쓰임이 시작됩니다.
class GpuBarrier final
{
public:
    GpuBarrier() = delete;

    static void Transition(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter);
};
