#pragma once

#include <DirectXMath.h>

// Camera의 위치와 바라볼 지점, 화면 비율을 보관하고 View와 Projection Matrix를 제공합니다.
class Camera final
{
public:
    Camera();

    void SetPosition(float x, float y, float z);
    void SetTarget(float x, float y, float z);
    // 화면의 가로 / 세로 비율입니다.
    void SetAspectRatio(float aspectRatio);
    void MoveForward(float distance);
    void MoveRight(float distance);
    void Rotate(float yawDelta, float pitchDelta);

    const DirectX::XMFLOAT3& GetPosition() const;
    float GetAspectRatio() const;
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;

private:
    DirectX::XMVECTOR GetForwardVector() const;
    void UpdateAnglesFromTarget();
    void UpdateTargetFromAngles();

    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT3 target_;
    DirectX::XMFLOAT3 up_;
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float aspectRatio_ = 1.0f;
};
