#include "ViewportPicker.h"

#include "Camera.h"
#include "GameObject.h"
#include "Ray.h"
#include "Scene.h"

#include <DirectXMath.h>

#include <cmath>
#include <limits>
#include <vector>

bool ViewportPicker::IsInside(
    const D3D12_VIEWPORT& viewport,
    const POINT& mousePosition)
{
    const float mouseX = static_cast<float>(mousePosition.x);
    const float mouseY = static_cast<float>(mousePosition.y);
    return viewport.Width > 0.0f &&
        viewport.Height > 0.0f &&
        mouseX >= viewport.TopLeftX &&
        mouseX < viewport.TopLeftX + viewport.Width &&
        mouseY >= viewport.TopLeftY &&
        mouseY < viewport.TopLeftY + viewport.Height;
}

bool ViewportPicker::Pick(
    const Scene& scene,
    const Camera& camera,
    const D3D12_VIEWPORT& viewport,
    const POINT& mousePosition,
    std::size_t& objectIndex)
{
    Ray ray{};
    if (!CreateRay(camera, viewport, mousePosition, ray))
    {
        return false;
    }

    bool isFound = false;
    float closestDistance = (std::numeric_limits<float>::max)();
    const std::vector<GameObject>& gameObjects = scene.GetGameObjects();
    for (std::size_t index = 0; index < gameObjects.size(); ++index)
    {
        const GameObject& gameObject = gameObjects[index];
        if (gameObject.GetModel() == nullptr)
        {
            continue;
        }

        float distance = 0.0f;
        if (ray.Intersects(gameObject.GetWorldBounds(), distance) &&
            distance < closestDistance)
        {
            closestDistance = distance;
            objectIndex = index;
            isFound = true;
        }
    }

    return isFound;
}

bool ViewportPicker::CreateRay(
    const Camera& camera,
    const D3D12_VIEWPORT& viewport,
    const POINT& mousePosition,
    Ray& ray)
{
    if (!IsInside(viewport, mousePosition))
    {
        return false;
    }

    const float mouseX = static_cast<float>(mousePosition.x);
    const float mouseY = static_cast<float>(mousePosition.y);
    const DirectX::XMMATRIX viewMatrix = camera.GetViewMatrix();
    const DirectX::XMMATRIX projectionMatrix = camera.GetProjectionMatrix();
    const DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();
    const DirectX::XMVECTOR screenNearPoint = DirectX::XMVectorSet(
        mouseX,
        mouseY,
        viewport.MinDepth,
        1.0f);
    const DirectX::XMVECTOR screenFarPoint = DirectX::XMVectorSet(
        mouseX,
        mouseY,
        viewport.MaxDepth,
        1.0f);

    const DirectX::XMVECTOR worldNearPoint = DirectX::XMVector3Unproject(
        screenNearPoint,
        viewport.TopLeftX,
        viewport.TopLeftY,
        viewport.Width,
        viewport.Height,
        viewport.MinDepth,
        viewport.MaxDepth,
        projectionMatrix,
        viewMatrix,
        worldMatrix);
    const DirectX::XMVECTOR worldFarPoint = DirectX::XMVector3Unproject(
        screenFarPoint,
        viewport.TopLeftX,
        viewport.TopLeftY,
        viewport.Width,
        viewport.Height,
        viewport.MinDepth,
        viewport.MaxDepth,
        projectionMatrix,
        viewMatrix,
        worldMatrix);
    const DirectX::XMVECTOR rayDirection = DirectX::XMVectorSubtract(
        worldFarPoint,
        worldNearPoint);
    const float directionLengthSquared = DirectX::XMVectorGetX(
        DirectX::XMVector3LengthSq(rayDirection));
    if (!std::isfinite(directionLengthSquared) ||
        directionLengthSquared <= 1.0e-12f)
    {
        return false;
    }

    ray.origin = camera.GetPosition();
    DirectX::XMStoreFloat3(
        &ray.direction,
        DirectX::XMVector3Normalize(rayDirection));
    return true;
}
