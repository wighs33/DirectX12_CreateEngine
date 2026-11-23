#pragma once

#include "ModelData.h"

#include <filesystem>

// glTF 2.0(.gltf + .bin) 파일을 이 엔진의 정점 형식과 왼손 좌표계로 바꿔 읽습니다.
class ModelLoader final
{
public:
    ModelLoader() = delete;

    // 실패하면 원인을 stderr에 출력하고 modelData를 바꾸지 않습니다.
    static bool LoadGltf(
        const std::filesystem::path& filePath,
        ModelData& modelData);
};
