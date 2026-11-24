#pragma once

#include "MaterialStyle.h"

#include <filesystem>
#include <map>
#include <string>

// 재질 이름마다 정한 그림체 값을 보관하고 파일과 맞춥니다.
// 같은 이름의 재질은 값 하나를 함께 쓰므로, 값을 고치면 그 재질을 쓰는 모든 물체가 바뀝니다.
class MaterialStyleLibrary final
{
public:
    MaterialStyleLibrary() = default;
    ~MaterialStyleLibrary() = default;

    MaterialStyleLibrary(const MaterialStyleLibrary&) = delete;
    MaterialStyleLibrary& operator=(const MaterialStyleLibrary&) = delete;

    // 읽지 못하면 이유를 출력하고 지금 값을 그대로 둡니다. 처음이면 모든 재질이 기본 그림체를 씁니다.
    bool Load(const std::filesystem::path& filePath);

    // 마지막에 Load한 파일을 다시 읽어, 저장하지 않은 값을 버립니다.
    bool Reload();

    // 마지막에 Load한 파일에 지금 값을 적습니다.
    bool Save();

    // 정해 둔 값이 없는 재질은 기본 그림체로 새로 만듭니다.
    // 돌려준 값의 자리는 이 객체가 살아 있는 동안 바뀌지 않습니다.
    MaterialStyle& FindOrAdd(const std::string& materialName);

    // 파일에 적힌 것과 다른 값이 있으면 true입니다.
    bool IsModified() const;

private:
    // 파일에 적힌 값이며, 적혀 있지 않으면 기본 그림체입니다.
    const MaterialStyle& FindSaved(const std::string& materialName) const;

    std::filesystem::path filePath_;
    std::map<std::string, MaterialStyle> styles_;
    std::map<std::string, MaterialStyle> savedStyles_;
};
