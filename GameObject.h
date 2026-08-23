#pragma once

#include "BoundingBox.h"
#include "Transform.h"

#include <string>

class Mesh;

enum class PrimitiveType
{
    Cube,
    Sphere
};

// Scene 안에서 공간 상태인 Transform과 렌더링 Geometry인 Mesh를 하나의 Object로 연결합니다.
class GameObject final
{
public:
    GameObject() = default;
    GameObject(Mesh* mesh, PrimitiveType primitiveType);

    void SetName(const std::string& name);
    const std::string& GetName() const;
    PrimitiveType GetPrimitiveType() const;

    void SetMesh(Mesh* mesh);

    Mesh* GetMesh();
    const Mesh* GetMesh() const;

    Transform& GetTransform();
    const Transform& GetTransform() const;
    AABB GetWorldBounds() const;

private:
    // Name은 Editor에 표시할 문자열이며 Object Identity나 Render 분기에는 사용하지 않습니다.
    std::string name_;

    // Transform은 GameObject마다 서로 다른 공간 상태를 가져야 하므로 값으로 소유합니다.
    Transform transform_;

    // Primitive Type은 Renderer 분기가 아닌 Scene Serialization용 Resource Metadata입니다.
    PrimitiveType primitiveType_ = PrimitiveType::Cube;

    // Mesh Resource의 수명은 외부에서 관리하며 GameObject는 사용 대상을 참조만 합니다.
    Mesh* mesh_ = nullptr;
};
