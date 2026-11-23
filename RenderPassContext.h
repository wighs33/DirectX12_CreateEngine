#pragma once

#include "SceneCuller.h"

#include <d3d12.h>

#include <vector>

class RenderStatistics;
class SceneConstants;
class ShadowMap;
class TextureHeap;

// 한 Frame 동안 모든 패스가 함께 쓰는 것을 모은 묶음입니다.
struct RenderPassContext
{
    ID3D12GraphicsCommandList* commandList;
    UINT frameIndex;
    // 화면을 그릴 때는 카메라에, 그림자를 그릴 때는 빛에 보이는 물체입니다.
    const std::vector<VisibleObject>& visibleObjects;
    const SceneConstants& constants;
    const TextureHeap& textureHeap;
    const ShadowMap& shadowMap;
    RenderStatistics& statistics;
};
