#pragma once

#include "ModelData.h"

// 파일 없이 코드로 기본 도형을 만듭니다.
class PrimitiveGeometry final
{
public:
    PrimitiveGeometry() = delete;

    static ModelData CreateCube();
    static ModelData CreateSphere();
    // 그림자를 받을 바닥입니다. 위에서만 보입니다.
    static ModelData CreatePlane();
};
