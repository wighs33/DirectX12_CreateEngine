#pragma once

#include <Windows.h>

class Camera;

// 오른쪽 버튼을 누른 동안 마우스와 W A S D로 카메라를 움직입니다.
class EditorCameraController final
{
public:
    // 오른쪽 버튼을 새로 눌렀거나 뗐는지 보고 둘러보기를 시작하거나 끝냅니다.
    void UpdateLookState(HWND window);

    // 둘러보는 중일 때만 카메라의 방향과 위치를 바꿉니다.
    void MoveCamera(Camera& camera, float deltaTime) const;

    bool IsLooking() const;

private:
    // UI 밖에서 오른쪽 버튼을 새로 누른 경우에만 켜집니다.
    bool isLooking_ = false;
};
