#pragma once

#include "MaterialStyle.h"

#include <filesystem>
#include <string>
#include <unordered_map>

// 재질 이름마다 정한 그림체 값을 JSON File에서 읽어 찾아 줍니다.
class MaterialStyleLibrary final
{
public:
    // 읽지 못하면 이유를 출력하고 모든 재질이 기본 그림체를 씁니다.
    bool Load(const std::filesystem::path& filePath);

    // 정해 둔 값이 없는 재질은 기본 그림체를 돌려줍니다.
    MaterialStyle Find(const std::string& materialName) const;

private:
    std::unordered_map<std::string, MaterialStyle> styles_;
};
