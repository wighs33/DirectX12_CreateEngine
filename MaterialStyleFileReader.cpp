#include "MaterialStyleFileReader.h"

#include "json.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>

namespace
{
    using Json = nlohmann::json;

    float ReadFloat(const Json& styleJson, const char* name, float defaultValue)
    {
        const float value = styleJson.value(name, defaultValue);
        if (!std::isfinite(value))
        {
            throw std::runtime_error(std::string("숫자가 아닙니다: ") + name);
        }
        return value;
    }

    DirectX::XMFLOAT3 ReadColor(
        const Json& styleJson,
        const char* name,
        const DirectX::XMFLOAT3& defaultValue)
    {
        const auto color = styleJson.find(name);
        if (color == styleJson.end())
        {
            return defaultValue;
        }
        if (!color->is_array() || color->size() != 3)
        {
            throw std::runtime_error(std::string("색은 숫자 세 개입니다: ") + name);
        }

        return DirectX::XMFLOAT3(
            color->at(0).get<float>(),
            color->at(1).get<float>(),
            color->at(2).get<float>());
    }

    MaterialStyle ReadStyle(const Json& styleJson)
    {
        MaterialStyle style;
        style.shadeColor = ReadColor(styleJson, "shadeColor", style.shadeColor);
        style.shadeThreshold =
            ReadFloat(styleJson, "shadeThreshold", style.shadeThreshold);
        style.shadeSoftness =
            ReadFloat(styleJson, "shadeSoftness", style.shadeSoftness);
        style.ambientWeight =
            ReadFloat(styleJson, "ambientWeight", style.ambientWeight);
        style.rimColor = ReadColor(styleJson, "rimColor", style.rimColor);
        style.rimStrength = ReadFloat(styleJson, "rimStrength", style.rimStrength);
        style.rimWidth = ReadFloat(styleJson, "rimWidth", style.rimWidth);
        style.outlineColor =
            ReadColor(styleJson, "outlineColor", style.outlineColor);
        style.outlineWidth =
            ReadFloat(styleJson, "outlineWidth", style.outlineWidth);
        return style;
    }
}

std::map<std::string, MaterialStyle> MaterialStyleFileReader::Read(
    const std::filesystem::path& filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        throw std::runtime_error("파일을 열 수 없습니다");
    }

    const Json document = Json::parse(input);
    if (document.at("version").get<int>() != MaterialStyleFormatVersion)
    {
        throw std::runtime_error("지원하지 않는 version입니다");
    }

    std::map<std::string, MaterialStyle> styles;
    for (const Json& styleJson : document.at("materials"))
    {
        const std::string name = styleJson.at("name").get<std::string>();
        if (!styles.emplace(name, ReadStyle(styleJson)).second)
        {
            throw std::runtime_error("이름이 같은 재질이 두 번 나옵니다: " + name);
        }
    }
    return styles;
}
