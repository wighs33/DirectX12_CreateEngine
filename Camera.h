#pragma once

#include <DirectXMath.h>

// Camera의 위치와 바라볼 지점을 보관하고 World Space를 View Space로 바꾸는 Matrix를 제공합니다.
class Camera final
{
public:
    Camera();

    void SetPosition(float x, float y, float z);
    void SetTarget(float x, float y, float z);
    void MoveForward(float distance);
    void MoveRight(float distance);
    void Rotate(float yawDelta, float pitchDelta);

    DirectX::XMMATRIX GetViewMatrix() const;

private:
    DirectX::XMVECTOR GetForwardVector() const;
    void UpdateAnglesFromTarget();
    void UpdateTargetFromAngles();

    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT3 target_;
    DirectX::XMFLOAT3 up_;
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
};
