#pragma once

#include "MaterialStyle.h"

#include <filesystem>
#include <map>
#include <string>

// 그림체를 다른 엔진도 읽을 수 있는 JSON File로 적습니다.
class MaterialStyleFileWriter final
{
public:
    MaterialStyleFileWriter() = delete;

    // 이름 순서로 적으며, 실패하면 이유를 담은 예외를 던집니다.
    // 쓰는 도중에 실패해도 이전 파일이 남도록 다른 이름으로 다 적은 뒤 바꿉니다.
    static void Write(
        const std::map<std::string, MaterialStyle>& styles,
        const std::filesystem::path& filePath);
};
