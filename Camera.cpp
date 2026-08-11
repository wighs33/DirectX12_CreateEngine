#include "Camera.h"

#include <algorithm>
#include <cmath>

Camera::Camera()
    : position_(0.0f, 0.0f, -3.0f),
      target_(0.0f, 0.0f, 0.0f),
      up_(0.0f, 1.0f, 0.0f)
{
}

void Camera::SetPosition(float x, float y, float z)
{
    position_ = DirectX::XMFLOAT3(x, y, z);
    UpdateAnglesFromTarget();
}

void Camera::SetTarget(float x, float y, float z)
{
    target_ = DirectX::XMFLOAT3(x, y, z);
    UpdateAnglesFromTarget();
}

void Camera::MoveForward(float distance)
{
    DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&position_);
    const DirectX::XMVECTOR movement = DirectX::XMVectorScale(
        GetForwardVector(),
        distance);
    position = DirectX::XMVectorAdd(position, movement);

    DirectX::XMStoreFloat3(&position_, position);
    UpdateTargetFromAngles();
}

void Camera::MoveRight(float distance)
{
    DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&position_);
    const DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&up_);

    // Up x Forward 순서이면 초기 +Z 시선에서 +X인 오른쪽 방향이 만들어집니다.
    const DirectX::XMVECTOR right = DirectX::XMVector3Cross(
        up,
        GetForwardVector());
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(right)) <= 0.0f)
    {
        return;
    }

    const DirectX::XMVECTOR movement = DirectX::XMVectorScale(
        DirectX::XMVector3Normalize(right),
        distance);
    position = DirectX::XMVectorAdd(position, movement);

    DirectX::XMStoreFloat3(&position_, position);
    UpdateTargetFromAngles();
}

void Camera::Rotate(float yawDelta, float pitchDelta)
{
    yaw_ = DirectX::XMScalarModAngle(yaw_ + yawDelta);

    constexpr float pitchLimit = DirectX::XM_PIDIV2 - DirectX::XM_PI / 180.0f;
    pitch_ = std::max(-pitchLimit, std::min(pitch_ + pitchDelta, pitchLimit));

    UpdateTargetFromAngles();
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
    // Position에서 Target을 바라보고 Up을 위쪽 기준으로 삼는 Left-Handed View Matrix를 만듭니다.
    const DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&position_);
    const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&target_);
    const DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&up_);

    return DirectX::XMMatrixLookAtLH(position, target, up);
}

DirectX::XMVECTOR Camera::GetForwardVector() const
{
    // Yaw 0, Pitch 0에서 기존 시선인 +Z를 바라보는 Left-Handed Forward를 만듭니다.
    const float cosPitch = std::cos(pitch_);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        cosPitch * std::sin(yaw_),
        std::sin(pitch_),
        cosPitch * std::cos(yaw_),
        0.0f);

    return DirectX::XMVector3Normalize(forward);
}

void Camera::UpdateAnglesFromTarget()
{
    const DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&position_);
    const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&target_);
    const DirectX::XMVECTOR direction = DirectX::XMVectorSubtract(target, position);

    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(direction)) <= 0.0f)
    {
        return;
    }

    const DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(direction);
    const float forwardY = std::max(
        -1.0f,
        std::min(DirectX::XMVectorGetY(forward), 1.0f));
    yaw_ = std::atan2(
        DirectX::XMVectorGetX(forward),
        DirectX::XMVectorGetZ(forward));
    pitch_ = std::asin(forwardY);
}

void Camera::UpdateTargetFromAngles()
{
    const DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&position_);
    const DirectX::XMVECTOR target = DirectX::XMVectorAdd(
        position,
        GetForwardVector());
    DirectX::XMStoreFloat3(&target_, target);
}
