#pragma once

#include "Frustum.h"

#include <DirectXMath.h>

#include <cstddef>
#include <vector>

class GameObject;
class RenderStatistics;
class Scene;

// 이번 Frame에 카메라에 보이는 물체입니다.
struct VisibleObject
{
    // Scene 목록에서의 번호이며 물체 상수 버퍼의 칸이기도 합니다.
    std::size_t objectIndex;
    const GameObject* gameObject;
};

// 카메라에 보이는 물체만 골라 모든 패스가 함께 쓸 목록을 만듭니다.
class SceneCuller final
{
public:
    void Collect(
        const Scene& scene,
        const DirectX::XMMATRIX& viewProjectionMatrix,
        RenderStatistics& statistics);

    // 다음 Collect 전까지, 장면의 물체가 바뀌지 않는 동안만 사용합니다.
    const std::vector<VisibleObject>& GetVisibleObjects() const;

private:
    Frustum frustum_;
    std::vector<VisibleObject> visibleObjects_;
};
