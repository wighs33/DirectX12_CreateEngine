#pragma once

#include "BoundingBox.h"

#include <DirectXMath.h>

// World Space에서 시작점과 정규화된 진행 방향으로 Picking Query를 표현합니다.
struct Ray
{
    DirectX::XMFLOAT3 origin{};
    DirectX::XMFLOAT3 direction{};
};

bool IntersectRayAABB(
    const Ray& ray,
    const AABB& bounds,
    float& intersectionDistance);
