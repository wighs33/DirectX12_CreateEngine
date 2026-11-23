#pragma once

#include <d3d12.h>

class Camera;
class EngineGuideWindow;
class GameObject;

// 고른 물체 위에 축을 그리고, 끌어서 바뀐 값을 물체에 넣습니다.
class TransformGizmo final
{
public:
    // W 이동, E 회전, R 크기를 고릅니다. 카메라를 움직이는 동안에는 무시합니다.
    void ProcessHotkeys(bool isCameraLooking);

    void DrawToolbar(EngineGuideWindow& guideWindow);

    // 고른 물체가 없으면 아무것도 그리지 않습니다.
    void Draw(
        GameObject* selectedObject,
        const Camera& camera,
        const D3D12_VIEWPORT& viewport,
        bool isCameraLooking);

    // 축 위에 마우스가 있거나 끄는 중이면 true입니다.
    static bool IsInteracting();

private:
    enum class Operation
    {
        Translate,
        Rotate,
        Scale
    };

    Operation operation_ = Operation::Translate;
};
