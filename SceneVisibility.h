#pragma once

#include "SceneCuller.h"

#include <DirectXMath.h>

#include <cstddef>
#include <vector>

class Scene;

// 이번 Frame에 카메라와 빛이 각각 보는 물체를 골라 담습니다.
class SceneVisibility final
{
public:
    void Collect(
        const Scene& scene,
        const DirectX::XMMATRIX& cameraViewProjection,
        const DirectX::XMMATRIX& lightViewProjection);

    // 화면에 그릴 물체입니다.
    const std::vector<VisibleObject>& GetCameraObjects() const;
    // 화면 밖이어도 그림자를 드리울 물체입니다.
    const std::vector<VisibleObject>& GetLightObjects() const;
    // 두 목록을 합쳐 한 번씩만 적은 것입니다.
    const std::vector<VisibleObject>& GetAllObjects() const;

    // 카메라의 시야 밖이라 빠진 물체의 수입니다.
    std::size_t GetCulledObjectCount() const;

private:
    void MergeObjects();

    SceneCuller cameraCuller_;
    SceneCuller lightCuller_;
    std::vector<VisibleObject> allObjects_;
};
