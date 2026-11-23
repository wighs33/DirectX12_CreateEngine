#pragma once

#include "ModelData.h"

// 파일 없이 코드로 기본 도형을 만듭니다.
class PrimitiveGeometry final
{
public:
    PrimitiveGeometry() = delete;

    static ModelData CreateCube();
    static ModelData CreateSphere();
};
