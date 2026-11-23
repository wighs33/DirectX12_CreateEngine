#include "GameObject.h"

#include "Model.h"

#include <cstddef>

GameObject::GameObject(const Model* model)
    : model_(model)
{
}

void GameObject::SetName(const std::string& name)
{
    name_ = name;
}

const std::string& GameObject::GetName() const
{
    return name_;
}

void GameObject::SetModel(const Model* model)
{
    model_ = model;
}

const Model* GameObject::GetModel() const
{
    return model_;
}

Transform& GameObject::GetTransform()
{
    return transform_;
}

const Transform& GameObject::GetTransform() const
{
    return transform_;
}

AABB GameObject::GetWorldBounds() const
{
    if (model_ == nullptr)
    {
        return {};
    }

    const AABB& localBounds = model_->GetLocalBounds();
    const std::array<DirectX::XMFLOAT3, 8> localCorners = localBounds.GetCorners();

    const DirectX::XMMATRIX worldMatrix = transform_.GetWorldMatrix();
    DirectX::XMVECTOR minimum = DirectX::XMVector3TransformCoord(
        DirectX::XMLoadFloat3(&localCorners[0]),
        worldMatrix);
    DirectX::XMVECTOR maximum = minimum;

    for (std::size_t cornerIndex = 1; cornerIndex < localCorners.size(); ++cornerIndex)
    {
        const DirectX::XMVECTOR worldCorner = DirectX::XMVector3TransformCoord(
            DirectX::XMLoadFloat3(&localCorners[cornerIndex]),
            worldMatrix);
        minimum = DirectX::XMVectorMin(minimum, worldCorner);
        maximum = DirectX::XMVectorMax(maximum, worldCorner);
    }

    AABB worldBounds{};
    DirectX::XMStoreFloat3(&worldBounds.minimum, minimum);
    DirectX::XMStoreFloat3(&worldBounds.maximum, maximum);
    return worldBounds;
}
