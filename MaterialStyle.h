#pragma once

#include <DirectXMath.h>

// 재질 하나의 카툰 그림체 값입니다. 색은 화면에서 고르는 값과 같은 sRGB이고 Unity 재질의 값과 뜻이 같습니다.
struct MaterialStyle
{
    // 그늘에서 바탕색에 곱하는 색입니다. 검게 누르지 않고 재질마다 다른 색을 냅니다.
    DirectX::XMFLOAT3 shadeColor{ 0.68f, 0.64f, 0.84f };
    // 빛과 면이 이루는 값(-1~1)이 이보다 작으면 그늘입니다.
    float shadeThreshold = 0.0f;
    // 밝은 면과 그늘의 경계가 번지는 폭입니다.
    float shadeSoftness = 0.03f;
    // 주변광을 더하는 정도입니다.
    float ambientWeight = 0.25f;
    DirectX::XMFLOAT3 rimColor{ 1.0f, 1.0f, 1.0f };
    float rimStrength = 0.2f;
    float rimWidth = 0.3f;
    DirectX::XMFLOAT3 outlineColor{ 0.12f, 0.10f, 0.16f };
    // 높이가 1080인 화면을 기준으로 한 픽셀 두께이며 0이면 외곽선을 그리지 않습니다.
    float outlineWidth = 2.5f;
};
