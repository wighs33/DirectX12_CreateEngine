#pragma once

#include "MaterialStyle.h"

#include <filesystem>
#include <map>
#include <string>

// 그림체 JSON File을 읽고 틀린 값이 없는지 검사합니다.
class MaterialStyleFileReader final
{
public:
    MaterialStyleFileReader() = delete;

    // 재질 이름으로 찾을 수 있게 돌려주며, 실패하면 이유를 담은 예외를 던집니다.
    // 적지 않은 값은 기본 그림체의 값을 그대로 씁니다.
    static std::map<std::string, MaterialStyle> Read(
        const std::filesystem::path& filePath);
};
