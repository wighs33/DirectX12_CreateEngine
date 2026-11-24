#pragma once

#include <d3d12.h>

// 그림이 놓일 자리(Viewport)와 그릴 수 있는 사각형(Scissor Rect)의 짝입니다.
class RenderArea final
{
public:
    RenderArea() = default;
    // 왼쪽 위부터 폭과 높이를 모두 쓰는 영역입니다.
    RenderArea(UINT width, UINT height);

    void Bind(ID3D12GraphicsCommandList* commandList) const;

    const D3D12_VIEWPORT& GetViewport() const;

private:
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
};
