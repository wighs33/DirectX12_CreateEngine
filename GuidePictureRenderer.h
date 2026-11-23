#pragma once

#include "EngineGuide.h"

// 카드에 적힌 상자, 선, 원, 글자를 그림판 위에 그립니다.
class GuidePictureRenderer final
{
public:
    GuidePictureRenderer() = delete;

    // position은 그림의 왼쪽 위이며 높이는 폭에 맞춰 정해집니다.
    static void Draw(
        const GuideCard& card,
        const GuidePoint& position,
        float width);

    static float GetHeight(float width);
};
