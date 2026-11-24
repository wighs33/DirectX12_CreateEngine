#pragma once

#include "SceneData.h"

#include <filesystem>

// JSON File을 읽고 빠지거나 틀린 값이 없는지 검사합니다.
class SceneFileReader final
{
public:
    SceneFileReader() = delete;

    // 실패하면 이유를 담은 예외를 던집니다.
    static SceneData Read(const std::filesystem::path& filePath);
};
