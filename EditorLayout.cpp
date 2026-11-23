#include "EditorLayout.h"

#include <algorithm>

namespace
{
    constexpr float PanelMargin = 10.0f;
    constexpr float PanelGap = 10.0f;
    constexpr float LeftPanelWidth = 220.0f;
    constexpr float RightPanelWidth = 280.0f;
    constexpr float CreatePanelHeight = 162.0f;
    constexpr float GizmoToolbarWidth = 320.0f;
    constexpr float GizmoToolbarHeight = 46.0f;
    constexpr float ViewportHelpWidth = 386.0f;
    constexpr float ViewportHelpHeight = 36.0f;
    constexpr float MinimumPanelHeight = 100.0f;

    void BeginFixedPanel(
        const char* title,
        const EditorLayout::Rect& rect,
        ImGuiWindowFlags additionalFlags)
    {
        ImGui::SetNextWindowPos(rect.position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(rect.size, ImGuiCond_Always);
        ImGui::Begin(
            title,
            nullptr,
            ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings |
                additionalFlags);
    }
}

EditorLayout::Rect EditorLayout::GetGizmoToolbar()
{
    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    return
    {
        ImVec2(
            viewport->WorkPos.x +
                (viewport->WorkSize.x - GizmoToolbarWidth) * 0.5f,
            viewport->WorkPos.y + PanelMargin),
        ImVec2(GizmoToolbarWidth, GizmoToolbarHeight)
    };
}

EditorLayout::Rect EditorLayout::GetCreatePanel()
{
    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    return
    {
        ImVec2(
            viewport->WorkPos.x + PanelMargin,
            viewport->WorkPos.y + PanelMargin),
        ImVec2(LeftPanelWidth, CreatePanelHeight)
    };
}

EditorLayout::Rect EditorLayout::GetHierarchyPanel()
{
    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    return
    {
        ImVec2(
            viewport->WorkPos.x + PanelMargin,
            viewport->WorkPos.y + PanelMargin + CreatePanelHeight + PanelGap),
        ImVec2(
            LeftPanelWidth,
            (std::max)(
                MinimumPanelHeight,
                viewport->WorkSize.y - CreatePanelHeight - PanelMargin * 3.0f))
    };
}

EditorLayout::Rect EditorLayout::GetInspectorPanel()
{
    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    return
    {
        ImVec2(
            (std::max)(
                viewport->WorkPos.x + PanelMargin,
                viewport->WorkPos.x + viewport->WorkSize.x -
                    RightPanelWidth - PanelMargin),
            viewport->WorkPos.y + PanelMargin),
        ImVec2(
            RightPanelWidth,
            (std::max)(
                MinimumPanelHeight,
                viewport->WorkSize.y - PanelMargin * 2.0f))
    };
}

EditorLayout::Rect EditorLayout::GetViewportHelpBar()
{
    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    return
    {
        ImVec2(
            viewport->WorkPos.x +
                (viewport->WorkSize.x - ViewportHelpWidth) * 0.5f,
            viewport->WorkPos.y + viewport->WorkSize.y -
                ViewportHelpHeight - PanelMargin),
        ImVec2(ViewportHelpWidth, ViewportHelpHeight)
    };
}

void EditorLayout::BeginPanel(const char* title, const Rect& rect)
{
    BeginFixedPanel(title, rect, ImGuiWindowFlags_None);
}

void EditorLayout::BeginPanelWithoutTitle(const char* name, const Rect& rect)
{
    BeginFixedPanel(name, rect, ImGuiWindowFlags_NoTitleBar);
}
