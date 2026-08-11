#pragma once

#include "BoundingBox.h"
#include "Transform.h"

class Mesh;

// Scene 안에서 공간 상태인 Transform과 렌더링 Geometry인 Mesh를 하나의 Object로 연결합니다.
class GameObject final
{
public:
    GameObject() = default;
    explicit GameObject(Mesh* mesh);

    void SetMesh(Mesh* mesh);

    Mesh* GetMesh();
    const Mesh* GetMesh() const;

    Transform& GetTransform();
    const Transform& GetTransform() const;
    AABB GetWorldBounds() const;

private:
    // Transform은 GameObject마다 서로 다른 공간 상태를 가져야 하므로 값으로 소유합니다.
    Transform transform_;

    // Mesh Resource의 수명은 외부에서 관리하며 GameObject는 사용 대상을 참조만 합니다.
    Mesh* mesh_ = nullptr;
};
