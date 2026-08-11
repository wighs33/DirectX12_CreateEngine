#include "Transform.h"

Transform::Transform()
    : position_(0.0f, 0.0f, 0.0f),
      rotation_(0.0f, 0.0f, 0.0f),
      scale_(1.0f, 1.0f, 1.0f)
{
}

void Transform::SetPosition(float x, float y, float z)
{
    position_ = DirectX::XMFLOAT3(x, y, z);
}

void Transform::SetRotation(float x, float y, float z)
{
    // Rotation은 DirectXMath와 바로 연결할 수 있도록 Radian 단위로 저장합니다.
    rotation_ = DirectX::XMFLOAT3(x, y, z);
}

void Transform::SetScale(float x, float y, float z)
{
    scale_ = DirectX::XMFLOAT3(x, y, z);
}

const DirectX::XMFLOAT3& Transform::GetPosition() const
{
    return position_;
}

const DirectX::XMFLOAT3& Transform::GetRotation() const
{
    return rotation_;
}

const DirectX::XMFLOAT3& Transform::GetScale() const
{
    return scale_;
}

DirectX::XMMATRIX Transform::GetWorldMatrix() const
{
    const DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(
        scale_.x,
        scale_.y,
        scale_.z);
    // 기존 Cube의 X -> Y 회전 순서를 유지하고 Z 회전을 마지막에 적용합니다.
    const DirectX::XMMATRIX rotationX = DirectX::XMMatrixRotationX(rotation_.x);
    const DirectX::XMMATRIX rotationY = DirectX::XMMatrixRotationY(rotation_.y);
    const DirectX::XMMATRIX rotationZ = DirectX::XMMatrixRotationZ(rotation_.z);
    const DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixMultiply(
        DirectX::XMMatrixMultiply(rotationX, rotationY),
        rotationZ);
    const DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(
        position_.x,
        position_.y,
        position_.z);

    // 현재 row-vector 흐름에서 Vertex에 Scale -> Rotation -> Translation 순서로 적용합니다.
    return DirectX::XMMatrixMultiply(
        DirectX::XMMatrixMultiply(scaleMatrix, rotationMatrix),
        translationMatrix);
}
