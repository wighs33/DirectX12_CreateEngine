#include "SceneLighting.h"

LightingConstants SceneLighting::CreateConstants(
    const DirectX::XMFLOAT3& cameraPosition) const
{
    // lightColor.w는 빛의 강도입니다.
    return LightingConstants
    {
        DirectX::XMFLOAT4(direction_.x, direction_.y, direction_.z, 0.0f),
        DirectX::XMFLOAT4(color_.x, color_.y, color_.z, intensity_),
        DirectX::XMFLOAT4(
            ambientColor_.x,
            ambientColor_.y,
            ambientColor_.z,
            0.0f),
        DirectX::XMFLOAT4(
            cameraPosition.x,
            cameraPosition.y,
            cameraPosition.z,
            1.0f)
    };
}
