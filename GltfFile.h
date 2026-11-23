#pragma once

#include "json.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// accessor가 가리키는 값의 시작 위치와 간격입니다. GltfFile이 살아 있는 동안만 사용합니다.
struct GltfAccessor
{
    const std::uint8_t* data = nullptr;
    std::size_t count = 0;
    std::size_t stride = 0;
    int componentType = 0;
    std::size_t componentCount = 0;
    bool isNormalized = false;
};

// .gltf 문서와 그 문서가 가리키는 .bin 파일을 읽고 값을 꺼내 줍니다.
// 파일이 잘못되었으면 이유를 담은 예외를 던집니다.
class GltfFile final
{
public:
    explicit GltfFile(const std::filesystem::path& filePath);

    const nlohmann::json& GetDocument() const;

    // 문서에 적힌 다른 파일의 경로를 구합니다.
    std::filesystem::path ResolveUri(const std::string& uri) const;

    // 값이 파일 범위를 벗어나지 않는지 확인하고 자리를 알려 줍니다.
    GltfAccessor GetAccessor(std::size_t accessorIndex) const;

    static float ReadFloat(
        const GltfAccessor& view,
        std::size_t elementIndex,
        std::size_t componentIndex);
    static std::uint32_t ReadIndex(
        const GltfAccessor& view,
        std::size_t elementIndex);

private:
    using Bytes = std::vector<std::uint8_t>;

    static Bytes ReadBinaryFile(const std::filesystem::path& filePath);

    std::filesystem::path directory_;
    nlohmann::json document_;
    std::vector<Bytes> buffers_;
};
