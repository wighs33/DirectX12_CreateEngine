#include "GameObject.h"

GameObject::GameObject(Mesh* mesh)
    : mesh_(mesh)
{
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
