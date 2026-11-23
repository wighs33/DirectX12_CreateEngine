#pragma once

class EditorSelection;
class EngineGuideWindow;
class Scene;
class Transform;

// 고른 물체의 위치, 회전, 크기를 숫자로 고치는 패널입니다.
class InspectorPanel final
{
public:
    InspectorPanel() = delete;

    static void Draw(
        Scene& scene,
        EditorSelection& selection,
        EngineGuideWindow& guideWindow);

private:
    static void DrawTransform(Transform& transform);
};
