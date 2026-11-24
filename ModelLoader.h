#pragma once

#include "ModelData.h"

#include <filesystem>

// 모델 파일을 읽고, 실패하면 이유를 알려 줍니다.
class ModelLoader final
{
public:
    ModelLoader() = delete;

    // glTF 2.0(.gltf + .bin) 파일을 읽습니다. 실패하면 원인을 stderr에 출력하고 modelData를 바꾸지 않습니다.
    static bool LoadGltf(
        const std::filesystem::path& filePath,
        ModelData& modelData);
};
