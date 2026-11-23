#include "MaterialStyleLibrary.h"

#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr int StyleFormatVersion = 1;

    using Json = nlohmann::json;

    bool ReportLoadError(
        const std::filesystem::path& filePath,
        const std::string& reason)
    {
        std::fprintf(
            stderr,
            "그림체 파일을 읽지 못했습니다: %s (%s)\n",
            filePath.u8string().c_str(),
            reason.c_str());
        return false;
    }

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

    // 적지 않은 값은 기본 그림체의 값을 그대로 씁니다.
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

bool MaterialStyleLibrary::Load(const std::filesystem::path& filePath)
{
    try
    {
        std::ifstream input(filePath);
        if (!input.is_open())
        {
            return ReportLoadError(filePath, "파일을 열 수 없습니다");
        }

        const Json document = Json::parse(input);
        if (document.at("version").get<int>() != StyleFormatVersion)
        {
            return ReportLoadError(filePath, "지원하지 않는 version입니다");
        }

        std::unordered_map<std::string, MaterialStyle> styles;
        for (const auto& item : document.at("materials").items())
        {
            styles[item.key()] = ReadStyle(item.value());
        }

        styles_ = std::move(styles);
        return true;
    }
    catch (const std::exception& exception)
    {
        return ReportLoadError(filePath, exception.what());
    }
}

MaterialStyle MaterialStyleLibrary::Find(const std::string& materialName) const
{
    const auto style = styles_.find(materialName);
    return style == styles_.end() ? MaterialStyle{} : style->second;
}
