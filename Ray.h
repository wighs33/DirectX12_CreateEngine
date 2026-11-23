#pragma once

#include "BoundingBox.h"

#include <DirectXMath.h>

// World Space에서 시작점과 정규화된 진행 방향으로 Picking Query를 표현합니다.
struct Ray
{
    DirectX::XMFLOAT3 origin{};
    DirectX::XMFLOAT3 direction{};

    // 상자에 닿으면 true를 반환하고 시작점에서 닿는 곳까지의 거리를 돌려줍니다.
    bool Intersects(const AABB& bounds, float& intersectionDistance) const;
};
