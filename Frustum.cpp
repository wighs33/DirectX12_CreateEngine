#include "Frustum.h"

#include <cstddef>

void Frustum::Update(const DirectX::XMMATRIX& viewProjectionMatrix)
{
    DirectX::XMFLOAT4X4 matrix{};
    DirectX::XMStoreFloat4x4(&matrix, viewProjectionMatrix);

    // Row-vector p * M에서 Clip 좌표의 각 성분은 M의 각 Column과 p의 Dot Product입니다.
    const DirectX::XMVECTOR column1 = DirectX::XMVectorSet(
        matrix._11, matrix._21, matrix._31, matrix._41);
    const DirectX::XMVECTOR column2 = DirectX::XMVectorSet(
        matrix._12, matrix._22, matrix._32, matrix._42);
    const DirectX::XMVECTOR column3 = DirectX::XMVectorSet(
        matrix._13, matrix._23, matrix._33, matrix._43);
    const DirectX::XMVECTOR column4 = DirectX::XMVectorSet(
        matrix._14, matrix._24, matrix._34, matrix._44);

    const auto storeNormalizedPlane = [this](PlaneIndex index, DirectX::XMVECTOR plane)
    {
        DirectX::XMStoreFloat4(
            &planes_[index],
            DirectX::XMPlaneNormalize(plane));
    };

    // DirectX Clip Space: -w <= x,y <= w, 0 <= z <= w. 내부는 Distance >= 0입니다.
    storeNormalizedPlane(LeftPlane, DirectX::XMVectorAdd(column4, column1));
    storeNormalizedPlane(RightPlane, DirectX::XMVectorSubtract(column4, column1));
    storeNormalizedPlane(BottomPlane, DirectX::XMVectorAdd(column4, column2));
    storeNormalizedPlane(TopPlane, DirectX::XMVectorSubtract(column4, column2));
    storeNormalizedPlane(NearPlane, column3);
    storeNormalizedPlane(FarPlane, DirectX::XMVectorSubtract(column4, column3));
}

bool Frustum::Intersects(const AABB& bounds) const
{
    const std::array<DirectX::XMFLOAT3, 8> corners = bounds.GetCorners();

    for (const DirectX::XMFLOAT4& storedPlane : planes_)
    {
        const DirectX::XMVECTOR plane = DirectX::XMLoadFloat4(&storedPlane);
        bool hasInsideCorner = false;

        for (const DirectX::XMFLOAT3& corner : corners)
        {
            const float distance = DirectX::XMVectorGetX(
                DirectX::XMPlaneDotCoord(
                    plane,
                    DirectX::XMLoadFloat3(&corner)));
            if (distance >= 0.0f)
            {
                hasInsideCorner = true;
                break;
            }
        }

        if (!hasInsideCorner)
        {
            return false;
        }
    }

    return true;
}
