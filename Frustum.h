#pragma once

#include "BoundingBox.h"

#include <DirectXMath.h>

#include <array>

// CPU 원본 ViewProjection에서 여섯 Plane을 만들고 World AABB의 가시성을 판정합니다.
class Frustum final
{
public:
    void Update(const DirectX::XMMATRIX& viewProjectionMatrix);
    bool Intersects(const AABB& bounds) const;

private:
    enum PlaneIndex
    {
        LeftPlane,
        RightPlane,
        TopPlane,
        BottomPlane,
        NearPlane,
        FarPlane,
        PlaneCount
    };

    std::array<DirectX::XMFLOAT4, PlaneCount> planes_{};
};
