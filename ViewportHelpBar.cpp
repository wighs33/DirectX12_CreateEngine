#include "ViewportHelpBar.h"

#include "EditorLayout.h"
#include "EngineGuideWindow.h"

#include "imgui.h"

void ViewportHelpBar::Draw(EngineGuideWindow& guideWindow)
{
    EditorLayout::BeginPanelWithoutTitle(
        "Viewport Help",
        EditorLayout::GetViewportHelpBar());
    DrawItem("Look (RMB)", "feature-camera", guideWindow);
    DrawItem("Pick (LMB)", "feature-picking", guideWindow);
    DrawItem("Stats", "feature-stats", guideWindow);
    DrawItem("Toon", "feature-toon", guideWindow);
    DrawItem("Shadow", "feature-shadow", guideWindow);
    DrawItem("Outline", "feature-outline", guideWindow);
    if (ImGui::SmallButton("Guide"))
    {
        guideWindow.OpenHome();
    }
    ImGui::End();
}

void ViewportHelpBar::DrawItem(
    const char* label,
    const char* guideId,
    EngineGuideWindow& guideWindow)
{
    ImGui::TextUnformatted(label);
    guideWindow.DrawHelpButton(guideId);
    ImGui::SameLine();
}
