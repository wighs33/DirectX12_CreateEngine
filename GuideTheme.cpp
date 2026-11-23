#include "GuideTheme.h"

#include <Windows.h>

#include <cfloat>
#include <cstdio>
#include <filesystem>
#include <system_error>

void GuideTheme::LoadFont()
{
    ImFontAtlas* const fonts = ImGui::GetIO().Fonts;

    // 첫 번째 글꼴이 기존 Panel에 쓰이므로 기본 글꼴을 먼저 등록해 모양을 유지합니다.
    fonts->AddFontDefault();

    wchar_t windowsDirectory[MAX_PATH]{};
    if (GetWindowsDirectoryW(windowsDirectory, MAX_PATH) == 0)
    {
        return;
    }

    const std::filesystem::path fontPath =
        std::filesystem::path(windowsDirectory) / "Fonts" / "malgun.ttf";
    std::error_code fileError;
    if (!std::filesystem::exists(fontPath, fileError))
    {
        std::fprintf(
            stderr,
            "한글 글꼴을 찾지 못했습니다: %s\n",
            fontPath.string().c_str());
        return;
    }

    font_ = fonts->AddFontFromFileTTF(
        fontPath.u8string().c_str(),
        BodyFontSize);
}

void GuideTheme::Push() const
{
    ImGui::PushFont(font_, BodyFontSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, BackgroundColor);
    ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
    ImGui::PushStyleColor(ImGuiCol_Button, PaperColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HairlineColor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 14.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));
}

void GuideTheme::Pop() const
{
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);
    ImGui::PopFont();
}

GuideTheme::PlateColors GuideTheme::GetPlateColors(const std::string& groupColor)
{
    if (groupColor == "flame")
        return { IM_COL32(90, 54, 35, 255), IM_COL32(67, 39, 25, 255) };
    if (groupColor == "violet")
        return { IM_COL32(61, 52, 112, 255), IM_COL32(45, 38, 86, 255) };
    if (groupColor == "teal")
        return { IM_COL32(31, 77, 71, 255), IM_COL32(22, 58, 54, 255) };
    if (groupColor == "rose")
        return { IM_COL32(92, 45, 70, 255), IM_COL32(69, 32, 51, 255) };

    return { IM_COL32(58, 64, 88, 255), IM_COL32(43, 48, 68, 255) };
}

ImVec2 GuideTheme::GetTextSize(
    float fontSize,
    const char* text,
    float wrapWidth)
{
    return ImGui::GetFont()->CalcTextSizeA(
        fontSize,
        FLT_MAX,
        wrapWidth,
        text);
}

void GuideTheme::DrawHeading(const char* text, float fontSize)
{
    ImGui::PushFont(nullptr, fontSize);
    ImGui::TextWrapped("%s", text);
    ImGui::PopFont();
}

void GuideTheme::DrawMutedText(const std::string& text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, MutedTextColor);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopStyleColor();
}

void GuideTheme::DrawSection(const char* heading, const std::string& text)
{
    DrawHeading(heading, HeadingFontSize);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::Spacing();
}
