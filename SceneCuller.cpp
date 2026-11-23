#include "SceneCuller.h"

#include "GameObject.h"
#include "Scene.h"

void SceneCuller::Collect(
    const Scene& scene,
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    // GPU 업로드용 Transpose Matrix가 아닌 CPU 원본 ViewProjection으로 Frame당 한 번 갱신합니다.
    frustum_.Update(viewProjectionMatrix);
    visibleObjects_.clear();
    culledObjectCount_ = 0;

    // Object를 직접 소유하지 않고 Scene이 제공하는 목록을 순회합니다.
    const std::vector<GameObject>& gameObjects = scene.GetGameObjects();
    for (std::size_t objectIndex = 0; objectIndex < gameObjects.size(); ++objectIndex)
    {
        const GameObject& gameObject = gameObjects[objectIndex];
        if (gameObject.GetModel() == nullptr)
        {
            continue;
        }

        if (!frustum_.Intersects(gameObject.GetWorldBounds()))
        {
            ++culledObjectCount_;
            continue;
        }

        visibleObjects_.push_back({ objectIndex, &gameObject });
    }
}

const std::vector<VisibleObject>& SceneCuller::GetVisibleObjects() const
{
    return visibleObjects_;
}

std::size_t SceneCuller::GetCulledObjectCount() const
{
    return culledObjectCount_;
}
