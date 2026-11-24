#include "ShadowCamera.h"

#include "BoundingBox.h"
#include "Camera.h"
#include "GameObject.h"
#include "Scene.h"
#include "ShadowSettings.h"

#include <algorithm>
#include <cmath>

void ShadowCamera::Update(const Scene& scene, const Camera& camera)
{
    AABB bounds;
    if (!CalculateSceneBounds(scene, bounds))
    {
        return;
    }

    const DirectX::XMVECTOR minimum = DirectX::XMLoadFloat3(&bounds.minimum);
    const DirectX::XMVECTOR maximum = DirectX::XMLoadFloat3(&bounds.maximum);
    DirectX::XMVECTOR center = DirectX::XMVectorScale(
        DirectX::XMVectorAdd(minimum, maximum),
        0.5f);
    float radius = 0.5f * DirectX::XMVectorGetX(
        DirectX::XMVector3Length(DirectX::XMVectorSubtract(maximum, minimum)));
    if (radius > ShadowSettings::MaximumRadius)
    {
        radius = ShadowSettings::MaximumRadius;
        center = DirectX::XMVectorAdd(
            DirectX::XMLoadFloat3(&camera.GetPosition()),
            DirectX::XMVectorScale(camera.GetForwardVector(), ShadowSettings::MaximumRadius));
    }
    radius = (std::max)(
        ShadowSettings::MinimumRadius,
        std::ceil(radius / ShadowSettings::RadiusStep) * ShadowSettings::RadiusStep);
    texelSize_ = radius * 2.0f / static_cast<float>(ShadowSettings::Resolution);

    // 빛의 방향을 앞으로 삼아 장면을 빛이 보는 축으로 돌립니다.
    const DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(
        DirectX::XMLoadFloat3(&scene.GetLighting().GetDirection()));
    const bool isVertical =
        std::abs(DirectX::XMVectorGetY(direction)) > 0.99f;
    const DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(
        DirectX::XMVectorZero(),
        direction,
        isVertical
            ? DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)
            : DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    // 시야가 움직여도 같은 자리가 같은 칸에 담기도록 중심을 칸 간격에 맞춥니다.
    // 맞추지 않으면 카메라를 움직일 때 그림자 가장자리가 자글거립니다.
    DirectX::XMFLOAT3 lightCenter{};
    DirectX::XMStoreFloat3(
        &lightCenter,
        DirectX::XMVector3TransformCoord(center, view));
    lightCenter.x = std::floor(lightCenter.x / texelSize_) * texelSize_;
    lightCenter.y = std::floor(lightCenter.y / texelSize_) * texelSize_;

    // 앞뒤로는 장면 전체를 담아, 시야보다 빛 쪽에 있는 물체의 그림자도 들어오게 합니다.
    float nearest = lightCenter.z - radius;
    float farthest = lightCenter.z + radius;
    for (const DirectX::XMFLOAT3& corner : bounds.GetCorners())
    {
        const float depth = DirectX::XMVectorGetZ(
            DirectX::XMVector3TransformCoord(
                DirectX::XMLoadFloat3(&corner),
                view));
        nearest = (std::min)(nearest, depth);
        farthest = (std::max)(farthest, depth);
    }

    const DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicOffCenterLH(
        lightCenter.x - radius,
        lightCenter.x + radius,
        lightCenter.y - radius,
        lightCenter.y + radius,
        nearest,
        farthest);
    DirectX::XMStoreFloat4x4(
        &viewProjection_,
        DirectX::XMMatrixMultiply(view, projection));
}

DirectX::XMMATRIX ShadowCamera::GetViewProjectionMatrix() const
{
    return DirectX::XMLoadFloat4x4(&viewProjection_);
}

float ShadowCamera::GetTexelSize() const
{
    return texelSize_;
}

bool ShadowCamera::CalculateSceneBounds(const Scene& scene, AABB& bounds)
{
    bool hasBounds = false;
    for (const GameObject& gameObject : scene.GetGameObjects())
    {
        if (gameObject.GetModel() == nullptr)
        {
            continue;
        }

        const AABB objectBounds = gameObject.GetWorldBounds();
        if (!hasBounds)
        {
            bounds = objectBounds;
            hasBounds = true;
            continue;
        }

        DirectX::XMStoreFloat3(
            &bounds.minimum,
            DirectX::XMVectorMin(
                DirectX::XMLoadFloat3(&bounds.minimum),
                DirectX::XMLoadFloat3(&objectBounds.minimum)));
        DirectX::XMStoreFloat3(
            &bounds.maximum,
            DirectX::XMVectorMax(
                DirectX::XMLoadFloat3(&bounds.maximum),
                DirectX::XMLoadFloat3(&objectBounds.maximum)));
    }
    return hasBounds;
}
