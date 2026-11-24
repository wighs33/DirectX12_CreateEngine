#pragma once

#include <d3d12.h>

// 그림자의 크기와 모양을 정하는 값을 한곳에 모읍니다.
class ShadowSettings final
{
public:
    ShadowSettings() = delete;

    // 그림자 맵 한 변의 칸 수입니다.
    static constexpr UINT Resolution = 2048;

    // 작은 장면이라도 이보다 좁히지 않아, 물체를 더해도 그림자 가장자리의 모양이 바뀌지 않습니다.
    static constexpr float MinimumRadius = 4.0f;
    // 담을 수 있는 가장 넓은 시야의 반지름입니다.
    static constexpr float MaximumRadius = 12.0f;
    // 물체가 조금 움직여도 한 칸의 크기가 바뀌지 않도록 반지름을 이 간격으로 올립니다.
    static constexpr float RadiusStep = 0.25f;

    // 그림자 맵 한 칸보다 작은 요철은 담기지 않으므로, 자기 표면에 그림자가 얼룩지지 않도록 칸의 몇 배를 밀어냅니다.
    // 시야의 반지름이 4일 때 Pandora Arena의 그림자와 같은 길이(약 3mm, 14mm)가 됩니다.
    static constexpr float DepthBias = 0.75f;
    static constexpr float NormalBias = 3.7f;
    // 빛과 나란한 면은 그림자 맵 한 칸 안에서도 깊이가 크게 바뀌므로 그만큼 뒤로 밉니다.
    static constexpr float SlopeScaledDepthBias = 5.0f;
};
