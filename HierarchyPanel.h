#pragma once

class EditorSelection;
class EngineGuideWindow;
class Scene;

// 장면에 있는 물체의 이름을 보여 주고 고르게 하는 패널입니다.
class HierarchyPanel final
{
public:
    HierarchyPanel() = delete;

    static void Draw(
        const Scene& scene,
        EditorSelection& selection,
        EngineGuideWindow& guideWindow);
};
