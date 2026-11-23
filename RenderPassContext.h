#pragma once

#include "SceneCuller.h"

#include <d3d12.h>

#include <vector>

class RenderStatistics;
class SceneConstants;
class TextureHeap;

// 한 Frame 동안 모든 패스가 함께 쓰는 것을 모은 묶음입니다.
struct RenderPassContext
{
    ID3D12GraphicsCommandList* commandList;
    UINT frameIndex;
    const std::vector<VisibleObject>& visibleObjects;
    const SceneConstants& constants;
    const TextureHeap& textureHeap;
    RenderStatistics& statistics;
};
