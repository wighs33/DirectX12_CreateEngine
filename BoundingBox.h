#pragma once

#include <DirectXMath.h>

#include <array>

// World 또는 Local Axis에 정렬된 Min/Max 두 점으로 공간 범위를 표현합니다.
struct AABB
{
    DirectX::XMFLOAT3 minimum{};
    DirectX::XMFLOAT3 maximum{};

    std::array<DirectX::XMFLOAT3, 8> GetCorners() const
    {
        return
        {
            DirectX::XMFLOAT3(minimum.x, minimum.y, minimum.z),
            DirectX::XMFLOAT3(maximum.x, minimum.y, minimum.z),
            DirectX::XMFLOAT3(minimum.x, maximum.y, minimum.z),
            DirectX::XMFLOAT3(maximum.x, maximum.y, minimum.z),
            DirectX::XMFLOAT3(minimum.x, minimum.y, maximum.z),
            DirectX::XMFLOAT3(maximum.x, minimum.y, maximum.z),
            DirectX::XMFLOAT3(minimum.x, maximum.y, maximum.z),
            DirectX::XMFLOAT3(maximum.x, maximum.y, maximum.z)
        };
    }
};
