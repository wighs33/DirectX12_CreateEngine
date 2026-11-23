#pragma once

#include "Frustum.h"

#include <DirectXMath.h>

#include <cstddef>
#include <vector>

class GameObject;
class Scene;

// 이번 Frame에 시야에 들어온 물체입니다.
struct VisibleObject
{
    // Scene 목록에서의 번호이며 물체 상수 버퍼의 칸이기도 합니다.
    std::size_t objectIndex;
    const GameObject* gameObject;
};

// 카메라나 빛의 시야에 들어온 물체만 골라 패스가 쓸 목록을 만듭니다.
class SceneCuller final
{
public:
    void Collect(
        const Scene& scene,
        const DirectX::XMMATRIX& viewProjectionMatrix);

    // 다음 Collect 전까지, 장면의 물체가 바뀌지 않는 동안만 사용합니다.
    const std::vector<VisibleObject>& GetVisibleObjects() const;

    // 시야 밖이라 빠진 물체의 수입니다.
    std::size_t GetCulledObjectCount() const;

private:
    Frustum frustum_;
    std::vector<VisibleObject> visibleObjects_;
    std::size_t culledObjectCount_ = 0;
};
