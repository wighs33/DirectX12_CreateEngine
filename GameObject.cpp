#include "GameObject.h"

#include "Mesh.h"

#include <cstddef>

GameObject::GameObject(Mesh* mesh, PrimitiveType primitiveType)
    : primitiveType_(primitiveType),
      mesh_(mesh)
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

PrimitiveType GameObject::GetPrimitiveType() const
{
    return primitiveType_;
}

void GameObject::SetMesh(Mesh* mesh)
{
    mesh_ = mesh;
}

Mesh* GameObject::GetMesh()
{
    return mesh_;
}

const Mesh* GameObject::GetMesh() const
{
    return mesh_;
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
    if (mesh_ == nullptr)
    {
        return {};
    }

    const AABB& localBounds = mesh_->GetLocalBounds();
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
