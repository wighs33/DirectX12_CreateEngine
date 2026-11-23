#pragma once

#include "imgui.h"

// 편집 화면에서 각 패널이 놓일 자리와 크기를 정합니다.
class EditorLayout final
{
public:
    struct Rect
    {
        ImVec2 position;
        ImVec2 size;
    };

    EditorLayout() = delete;

    static Rect GetGizmoToolbar();
    static Rect GetCreatePanel();
    static Rect GetHierarchyPanel();
    static Rect GetInspectorPanel();
    static Rect GetViewportHelpBar();

    // 정해진 자리에 움직이지 않는 패널을 시작합니다. 끝날 때 ImGui::End를 부릅니다.
    static void BeginPanel(const char* title, const Rect& rect);
    static void BeginPanelWithoutTitle(const char* name, const Rect& rect);
};
