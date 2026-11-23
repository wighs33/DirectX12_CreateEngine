#include "GltfFile.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace
{
    using Json = nlohmann::json;

    // glTF 2.0 명세의 accessor.componentType 값입니다.
    constexpr int ComponentUnsignedByte = 5121;
    constexpr int ComponentUnsignedShort = 5123;
    constexpr int ComponentUnsignedInt = 5125;
    constexpr int ComponentFloat = 5126;

    std::size_t GetComponentSize(int componentType)
    {
        switch (componentType)
        {
        case ComponentUnsignedByte:
            return 1;
        case ComponentUnsignedShort:
            return 2;
        case ComponentUnsignedInt:
        case ComponentFloat:
            return 4;
        }

        throw std::runtime_error("지원하지 않는 componentType입니다");
    }

    std::size_t GetComponentCount(const std::string& typeName)
    {
        if (typeName == "SCALAR")
            return 1;
        if (typeName == "VEC2")
            return 2;
        if (typeName == "VEC3")
            return 3;
        if (typeName == "VEC4")
            return 4;

        throw std::runtime_error("지원하지 않는 accessor type입니다: " + typeName);
    }

    // uri에 들어 있는 %20 같은 표기를 원래 글자로 되돌립니다.
    std::string DecodeUri(const std::string& uri)
    {
        std::string result;
        for (std::size_t index = 0; index < uri.size(); ++index)
        {
            if (uri[index] == '%' && index + 2 < uri.size())
            {
                result.push_back(static_cast<char>(
                    std::stoi(uri.substr(index + 1, 2), nullptr, 16)));
                index += 2;
            }
            else
            {
                result.push_back(uri[index]);
            }
        }

        return result;
    }
}

GltfFile::GltfFile(const std::filesystem::path& filePath)
    : directory_(filePath.parent_path())
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        throw std::runtime_error("파일을 열 수 없습니다");
    }

    document_ = Json::parse(input);
    for (const Json& buffer : document_.value("buffers", Json::array()))
    {
        buffers_.push_back(ReadBinaryFile(
            ResolveUri(buffer.at("uri").get<std::string>())));
    }
}

const nlohmann::json& GltfFile::GetDocument() const
{
    return document_;
}

std::filesystem::path GltfFile::ResolveUri(const std::string& uri) const
{
    if (uri.rfind("data:", 0) == 0)
    {
        throw std::runtime_error(
            "파일 안에 넣은 데이터(data:)는 지원하지 않습니다. 파일을 나눠 내보내세요");
    }

    return directory_ / std::filesystem::u8path(DecodeUri(uri));
}

GltfAccessor GltfFile::GetAccessor(std::size_t accessorIndex) const
{
    const Json& accessor = document_.at("accessors").at(accessorIndex);
    if (accessor.find("sparse") != accessor.end() ||
        accessor.find("bufferView") == accessor.end())
    {
        throw std::runtime_error("sparse accessor는 지원하지 않습니다");
    }

    const Json& bufferView = document_.at("bufferViews")
        .at(accessor.at("bufferView").get<std::size_t>());
    const Bytes& buffer =
        buffers_.at(bufferView.at("buffer").get<std::size_t>());

    GltfAccessor view;
    view.count = accessor.at("count").get<std::size_t>();
    view.componentType = accessor.at("componentType").get<int>();
    view.componentCount =
        GetComponentCount(accessor.at("type").get<std::string>());
    view.isNormalized = accessor.value("normalized", false);

    const std::size_t elementSize =
        GetComponentSize(view.componentType) * view.componentCount;
    view.stride = bufferView.value("byteStride", elementSize);

    // 파일이 잘못되어도 Buffer 밖을 읽지 않도록 마지막 값의 끝까지 확인합니다.
    const std::uint64_t offset =
        bufferView.value("byteOffset", static_cast<std::uint64_t>(0)) +
        accessor.value("byteOffset", static_cast<std::uint64_t>(0));
    const std::uint64_t end = view.count == 0
        ? offset
        : offset +
            static_cast<std::uint64_t>(view.count - 1) * view.stride +
            elementSize;
    if (end > buffer.size())
    {
        throw std::runtime_error("accessor가 buffer 범위를 벗어났습니다");
    }

    view.data = buffer.data() + static_cast<std::size_t>(offset);
    return view;
}

float GltfFile::ReadFloat(
    const GltfAccessor& view,
    std::size_t elementIndex,
    std::size_t componentIndex)
{
    const std::uint8_t* const source = view.data +
        elementIndex * view.stride +
        componentIndex * GetComponentSize(view.componentType);
    switch (view.componentType)
    {
    case ComponentFloat:
    {
        float value = 0.0f;
        std::memcpy(&value, source, sizeof(value));
        return value;
    }
    case ComponentUnsignedByte:
        return static_cast<float>(*source) /
            (view.isNormalized ? 255.0f : 1.0f);
    case ComponentUnsignedShort:
    {
        std::uint16_t value = 0;
        std::memcpy(&value, source, sizeof(value));
        return static_cast<float>(value) /
            (view.isNormalized ? 65535.0f : 1.0f);
    }
    }

    throw std::runtime_error("지원하지 않는 정점 값 형식입니다");
}

std::uint32_t GltfFile::ReadIndex(
    const GltfAccessor& view,
    std::size_t elementIndex)
{
    const std::uint8_t* const source =
        view.data + elementIndex * view.stride;
    switch (view.componentType)
    {
    case ComponentUnsignedByte:
        return *source;
    case ComponentUnsignedShort:
    {
        std::uint16_t value = 0;
        std::memcpy(&value, source, sizeof(value));
        return value;
    }
    case ComponentUnsignedInt:
    {
        std::uint32_t value = 0;
        std::memcpy(&value, source, sizeof(value));
        return value;
    }
    }

    throw std::runtime_error("지원하지 않는 인덱스 형식입니다");
}

GltfFile::Bytes GltfFile::ReadBinaryFile(
    const std::filesystem::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open())
    {
        throw std::runtime_error(
            "파일을 열 수 없습니다: " + filePath.u8string());
    }

    return Bytes(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}
