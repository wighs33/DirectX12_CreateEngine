#include "EditorHelp.h"

#include "imgui.h"

#include <Windows.h>
#include <shellapi.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace
{
    // 카드마다 '#카드이름' 주소를 가진 엔진 도감 페이지입니다.
    constexpr char CardPageUrl[] = "https://claude.ai/artifact/T6qhnGLtAeJtfp5JgGGBaY";
}

void EditorHelp::DrawButton(const char* cardId)
{
    ImGui::SameLine();
    DrawButtonOnCurrentLine(cardId);
}

void EditorHelp::DrawButtonAtRight(const char* cardId)
{
    // 남은 폭을 기준으로 계산하므로 Scrollbar가 생겨도 물음표가 가려지지 않습니다.
    ImGui::SameLine();
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        ImGui::GetContentRegionAvail().x -
        GetButtonWidth());
    DrawButtonOnCurrentLine(cardId);
}

float EditorHelp::GetReservedWidth()
{
    return GetButtonWidth() + ImGui::GetStyle().ItemSpacing.x;
}

float EditorHelp::GetButtonWidth()
{
    return ImGui::CalcTextSize("?").x +
        ImGui::GetStyle().FramePadding.x * 2.0f;
}

void EditorHelp::DrawButtonOnCurrentLine(const char* cardId)
{
    // 글자가 모두 '?'로 같으므로 카드 이름을 ID로 사용해 Button을 구분합니다.
    ImGui::PushID(cardId);
    if (ImGui::SmallButton("?"))
    {
        OpenCard(cardId);
    }
    ImGui::PopID();
}

void EditorHelp::OpenCard(const char* cardId)
{
    const std::string url = std::string(CardPageUrl) + "#" + cardId;

    // ShellExecute는 성공하면 32보다 큰 값을 반환합니다.
    const auto result = reinterpret_cast<std::intptr_t>(ShellExecuteA(
        nullptr,
        "open",
        url.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL));
    if (result <= 32)
    {
        std::fprintf(
            stderr,
            "카드 페이지를 열지 못했습니다: %s (ShellExecute 결과: %d)\n",
            url.c_str(),
            static_cast<int>(result));
    }
}
