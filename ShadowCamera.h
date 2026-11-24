#pragma once

#include <DirectXMath.h>

class Camera;
class Scene;
struct AABB;

// 빛이 장면을 바라보는 시점입니다. 빛은 나란히 오므로 원근이 없는 상자 모양의 시야를 씁니다.
class ShadowCamera final
{
public:
    // 장면의 물체가 모두 들어오도록 시야를 맞춥니다.
    // 장면이 아주 넓으면 카메라 앞쪽만 담아 그림자가 거칠어지지 않게 합니다.
    void Update(const Scene& scene, const Camera& camera);

    DirectX::XMMATRIX GetViewProjectionMatrix() const;

    // 그림자 맵 한 칸이 장면에서 차지하는 길이입니다.
    float GetTexelSize() const;

private:
    // 모델이 있는 물체를 모두 감싸는 상자를 구하고, 물체가 없으면 false를 반환합니다.
    static bool CalculateSceneBounds(const Scene& scene, AABB& bounds);

    DirectX::XMFLOAT4X4 viewProjection_
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    float texelSize_ = 0.0f;
};
