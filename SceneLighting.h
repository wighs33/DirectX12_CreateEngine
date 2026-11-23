#pragma once

#include <DirectXMath.h>

// Shader의 b1 앞부분과 같은 배치로 보낼 값입니다.
struct LightingConstants
{
    DirectX::XMFLOAT4 lightDirection;
    DirectX::XMFLOAT4 lightColor;
    DirectX::XMFLOAT4 ambientColor;
};

// 장면 전체를 비추는 빛 하나와 주변광을 보관합니다.
class SceneLighting final
{
public:
    LightingConstants CreateConstants() const;

    // 빛이 진행하는 방향입니다.
    const DirectX::XMFLOAT3& GetDirection() const;

private:
    DirectX::XMFLOAT3 direction_{ 0.35f, -0.75f, 0.55f };
    // 빛과 주변광의 색은 Pandora Arena 장면의 값을 선형으로 바꾼 것입니다.
    DirectX::XMFLOAT3 color_{ 1.0f, 0.888f, 0.730f };
    float intensity_ = 1.0f;
    DirectX::XMFLOAT3 ambientColor_{ 0.46f, 0.42f, 0.46f };
};
