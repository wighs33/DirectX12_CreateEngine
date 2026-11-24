#include "RenderArea.h"

RenderArea::RenderArea(UINT width, UINT height)
{
    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.MinDepth = D3D12_MIN_DEPTH;
    viewport_.MaxDepth = D3D12_MAX_DEPTH;

    scissorRect_.left = 0;
    scissorRect_.top = 0;
    scissorRect_.right = static_cast<LONG>(width);
    scissorRect_.bottom = static_cast<LONG>(height);
}

void RenderArea::Bind(ID3D12GraphicsCommandList* commandList) const
{
    // [7단계] Rasterizer가 Clip Space 결과를 변환할 화면 영역을 설정합니다.
    commandList->RSSetViewports(1, &viewport_);

    // [8단계] Rasterizer가 실제 Pixel을 통과시킬 사각 영역을 설정합니다.
    commandList->RSSetScissorRects(1, &scissorRect_);
}

const D3D12_VIEWPORT& RenderArea::GetViewport() const
{
    return viewport_;
}
