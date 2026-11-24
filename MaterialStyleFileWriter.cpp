#include "MaterialStyleFileWriter.h"

#include "json.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
    using Json = nlohmann::json;

    // 0.36이 0.36000001로 적히지 않도록 소수점 아래 네 자리까지만 씁니다.
    std::string FormatFloat(float value)
    {
        const double rounded = std::round(value * 10000.0) / 10000.0;
        return Json(rounded).dump();
    }

    std::string FormatColor(const DirectX::XMFLOAT3& color)
    {
        return "[" + FormatFloat(color.x) + ", " + FormatFloat(color.y) +
            ", " + FormatFloat(color.z) + "]";
    }

    void WriteStyle(
        std::ostream& output,
        const std::string& name,
        const MaterialStyle& style)
    {
        const auto writeValue = [&output](
            const char* key,
            const std::string& value,
            bool isLast = false)
        {
            output << "            \"" << key << "\": " << value
                << (isLast ? "\n" : ",\n");
        };

        output << "        {\n";
        writeValue("name", Json(name).dump());
        writeValue("shadeColor", FormatColor(style.shadeColor));
        writeValue("shadeThreshold", FormatFloat(style.shadeThreshold));
        writeValue("shadeSoftness", FormatFloat(style.shadeSoftness));
        writeValue("ambientWeight", FormatFloat(style.ambientWeight));
        writeValue("rimColor", FormatColor(style.rimColor));
        writeValue("rimStrength", FormatFloat(style.rimStrength));
        writeValue("rimWidth", FormatFloat(style.rimWidth));
        writeValue("outlineColor", FormatColor(style.outlineColor));
        writeValue("outlineWidth", FormatFloat(style.outlineWidth), true);
        output << "        }";
    }
}

void MaterialStyleFileWriter::Write(
    const std::map<std::string, MaterialStyle>& styles,
    const std::filesystem::path& filePath)
{
    std::filesystem::path temporaryPath = filePath;
    temporaryPath += ".tmp";

    std::ofstream output(temporaryPath, std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error("파일을 열 수 없습니다");
    }

    output << "{\n    \"version\": " << MaterialStyleFormatVersion
        << ",\n    \"materials\": [\n";
    bool isFirst = true;
    for (const auto& [name, style] : styles)
    {
        if (!isFirst)
        {
            output << ",\n";
        }
        WriteStyle(output, name, style);
        isFirst = false;
    }
    output << "\n    ]\n}\n";
    output.close();
    if (!output.good())
    {
        throw std::runtime_error("파일에 다 적지 못했습니다");
    }

    std::error_code renameError;
    std::filesystem::rename(temporaryPath, filePath, renameError);
    if (renameError)
    {
        throw std::runtime_error(
            "파일을 바꾸지 못했습니다: " + renameError.message());
    }
}
