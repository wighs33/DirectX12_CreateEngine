#pragma once

#include <DirectXMath.h>

// Render Object의 위치, 회전, 크기를 보관하고 Object Space를 World Space로 바꾸는 Matrix를 제공합니다.
class Transform final
{
public:
    Transform();

    void SetPosition(float x, float y, float z);
    void SetRotation(float x, float y, float z);
    void SetScale(float x, float y, float z);

    const DirectX::XMFLOAT3& GetPosition() const;
    const DirectX::XMFLOAT3& GetRotation() const;
    const DirectX::XMFLOAT3& GetScale() const;

    DirectX::XMMATRIX GetWorldMatrix() const;

private:
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT3 rotation_;
    DirectX::XMFLOAT3 scale_;
};
