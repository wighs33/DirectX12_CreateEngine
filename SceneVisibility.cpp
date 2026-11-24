#include "SceneVisibility.h"

#include <algorithm>
#include <iterator>

void SceneVisibility::Collect(
    const Scene& scene,
    const DirectX::XMMATRIX& cameraViewProjection,
    const DirectX::XMMATRIX& lightViewProjection)
{
    cameraCuller_.Collect(scene, cameraViewProjection);
    lightCuller_.Collect(scene, lightViewProjection);
    MergeObjects();
}

const std::vector<VisibleObject>& SceneVisibility::GetCameraObjects() const
{
    return cameraCuller_.GetVisibleObjects();
}

const std::vector<VisibleObject>& SceneVisibility::GetLightObjects() const
{
    return lightCuller_.GetVisibleObjects();
}

const std::vector<VisibleObject>& SceneVisibility::GetAllObjects() const
{
    return allObjects_;
}

std::size_t SceneVisibility::GetCulledObjectCount() const
{
    return cameraCuller_.GetCulledObjectCount();
}

void SceneVisibility::MergeObjects()
{
    const std::vector<VisibleObject>& cameraObjects = GetCameraObjects();
    const std::vector<VisibleObject>& lightObjects = GetLightObjects();

    // 두 목록 모두 Scene의 순서대로이므로 번호를 견주어 합칩니다.
    allObjects_.clear();
    std::set_union(
        cameraObjects.begin(),
        cameraObjects.end(),
        lightObjects.begin(),
        lightObjects.end(),
        std::back_inserter(allObjects_),
        [](const VisibleObject& left, const VisibleObject& right)
        {
            return left.objectIndex < right.objectIndex;
        });
}
