#pragma once

#include "EngineGuideData.h"

#include <filesystem>

// 도감의 JSON File을 카드, 기능, 묶음으로 바꿉니다.
class EngineGuideReader final
{
public:
    EngineGuideReader() = delete;

    // 실패하면 이유를 담은 예외를 던집니다.
    static EngineGuideData Read(const std::filesystem::path& filePath);
};
