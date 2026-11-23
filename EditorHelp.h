#pragma once

// Editor 기능 옆의 물음표 Button을 그리고, 누르면 엔진 도감의 카드 설명 페이지를 엽니다.
class EditorHelp final
{
public:
    EditorHelp() = delete;

    // 바로 앞에 그린 UI와 같은 줄의 오른쪽에 물음표를 이어 그립니다.
    static void DrawButton(const char* cardId);

    // 바로 앞에 그린 UI와 같은 줄을 유지하되 Panel 오른쪽 끝에 맞춰 물음표를 그립니다.
    static void DrawButtonAtRight(const char* cardId);

    // Panel 폭을 채우는 UI가 물음표 자리를 남겨 둘 때 사용하는 폭입니다.
    static float GetReservedWidth();

private:
    static float GetButtonWidth();
    static void DrawButtonOnCurrentLine(const char* cardId);
    static void OpenCard(const char* cardId);
};
