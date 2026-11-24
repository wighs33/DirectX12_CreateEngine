#pragma once

#include "VertexLayout.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// 파일에서 읽은 재질입니다. Texture가 없으면 경로가 비어 있고 정점 색만 사용합니다.
struct ModelMaterialData
{
    std::string name;
    std::filesystem::path baseColorTexturePath;
};

// 모델을 이루는 조각 하나이며 재질 하나를 사용합니다.
struct ModelPartData
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t materialIndex = 0;
};

// GPU Resource를 만들기 전의 CPU 모델 데이터입니다.
struct ModelData
{
    std::vector<ModelMaterialData> materials;
    std::vector<ModelPartData> parts;
};
