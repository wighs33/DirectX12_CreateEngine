#include "EngineGuideWindow.h"

#include "GuideCardPage.h"
#include "GuideFeaturePage.h"
#include "GuideHomePage.h"
#include "GuideNavigationBar.h"

#include "imgui.h"

#include <algorithm>
#include <cfloat>

void EngineGuideWindow::Initialize(const std::filesystem::path& guideFilePath)
{
    theme_.LoadFont();
    isLoaded_ = guide_.Load(guideFilePath);
}

void EngineGuideWindow::OpenHome()
{
    Open(std::string());
}

void EngineGuideWindow::Open(const std::string& id)
{
    isOpen_ = true;
    isFocusRequested_ = true;
    navigation_.Request(id);
}

void EngineGuideWindow::Draw()
{
    if (!isOpen_)
    {
        return;
    }

    isScrollResetRequested_ = navigation_.Apply() || isScrollResetRequested_;

    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        viewport->GetWorkCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(
        ImVec2(
            (std::min)(1180.0f, viewport->WorkSize.x - 40.0f),
            (std::min)(640.0f, viewport->WorkSize.y - 40.0f)),
        ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(380.0f, 300.0f),
        ImVec2(FLT_MAX, FLT_MAX));
    if (isFocusRequested_)
    {
        ImGui::SetNextWindowFocus();
        isFocusRequested_ = false;
    }

    theme_.Push();

    // '###' 뒤의 이름만 창을 구분하므로 보이는 제목은 자유롭게 바꿀 수 있습니다.
    if (ImGui::Begin(
            "Pandora Engine 도감###EngineGuide",
            &isOpen_,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
    {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            isOpen_ = false;
        }

        GuideNavigationBar::Draw(guide_, navigation_);
        ImGui::Separator();

        if (ImGui::BeginChild("GuideContent"))
        {
            if (isScrollResetRequested_)
            {
                ImGui::SetScrollY(0.0f);
                isScrollResetRequested_ = false;
            }

            DrawPage();
        }
        ImGui::EndChild();
    }
    ImGui::End();

    theme_.Pop();
}

void EngineGuideWindow::DrawPage()
{
    if (!isLoaded_)
    {
        ImGui::TextWrapped(
            "설명 파일을 읽지 못했습니다. Assets/Guide/EngineGuide.json을 확인하세요.");
        return;
    }

    const std::string& currentId = navigation_.GetCurrentId();
    if (const GuideCard* const card = guide_.FindCard(currentId))
    {
        GuideCardPage::Draw(*card, guide_, navigation_);
    }
    else if (const GuideFeature* const feature = guide_.FindFeature(currentId))
    {
        GuideFeaturePage::Draw(*feature, guide_, navigation_);
    }
    else
    {
        GuideHomePage::Draw(guide_, navigation_);
    }
}
