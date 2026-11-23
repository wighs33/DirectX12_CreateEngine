#pragma once

#include "imgui.h"

#include <string>

// 도감 창의 색과 글자 크기를 정하고 같은 모양의 글을 그립니다.
class GuideTheme final
{
public:
    // 카드 그림판은 셀 셰이딩처럼 밝은 면과 그늘 두 단으로만 나눕니다.
    struct PlateColors
    {
        ImU32 lit;
        ImU32 shade;
    };

    // ImGui의 글꼴 크기는 글자의 줄 높이이므로 일반적인 글자 크기보다 크게 지정합니다.
    static constexpr float BodyFontSize = 20.0f;
    static constexpr float SmallFontSize = 18.0f;
    static constexpr float HeadingFontSize = 26.0f;
    static constexpr float TitleFontSize = 36.0f;

    static constexpr ImU32 BackgroundColor = IM_COL32(22, 18, 31, 255);
    static constexpr ImU32 PaperColor = IM_COL32(34, 28, 48, 255);
    static constexpr ImU32 TextColor = IM_COL32(240, 236, 250, 255);
    static constexpr ImU32 MutedTextColor = IM_COL32(176, 166, 200, 255);
    static constexpr ImU32 LineColor = IM_COL32(205, 195, 234, 255);
    static constexpr ImU32 HairlineColor = IM_COL32(64, 55, 90, 255);
    static constexpr ImU32 FlameColor = IM_COL32(255, 138, 76, 255);
    static constexpr ImU32 BoxColor = IM_COL32(44, 37, 64, 255);
    static constexpr ImU32 HighlightColor = IM_COL32(106, 90, 31, 255);

    // 첫 Frame 전에 한 번 불러 한글 글꼴을 등록합니다.
    void LoadFont();

    // 도감 창을 그리는 동안만 글꼴과 색을 바꿉니다.
    void Push() const;
    void Pop() const;

    static PlateColors GetPlateColors(const std::string& groupColor);
    static ImVec2 GetTextSize(
        float fontSize,
        const char* text,
        float wrapWidth = 0.0f);

    static void DrawHeading(const char* text, float fontSize);
    static void DrawMutedText(const std::string& text);
    // 작은 제목과 그 아래 설명을 함께 그립니다.
    static void DrawSection(const char* heading, const std::string& text);

private:
    // 소유권은 ImGui Font Atlas에 있으며 글꼴이 없으면 기본 글꼴로 그립니다.
    ImFont* font_ = nullptr;
};
