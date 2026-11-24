#include "MaterialStyleLibrary.h"

#include "MaterialStyleFileReader.h"
#include "MaterialStyleFileWriter.h"

#include <cstdio>
#include <exception>
#include <utility>

namespace
{
    const MaterialStyle DefaultStyle;
}

bool MaterialStyleLibrary::Load(const std::filesystem::path& filePath)
{
    filePath_ = filePath;
    return Reload();
}

bool MaterialStyleLibrary::Reload()
{
    try
    {
        savedStyles_ = MaterialStyleFileReader::Read(filePath_);

        // 재질이 값의 자리를 기억하고 있으므로 지우지 않고 그 자리에 다시 적습니다.
        for (auto& [name, style] : styles_)
        {
            style = FindSaved(name);
        }
        for (const auto& [name, style] : savedStyles_)
        {
            styles_.emplace(name, style);
        }
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "그림체 파일을 읽지 못했습니다: %s (%s)\n",
            filePath_.u8string().c_str(),
            exception.what());
        return false;
    }
}

bool MaterialStyleLibrary::Save()
{
    try
    {
        // 고치지 않아 기본 그림체 그대로인 재질은 적지 않습니다.
        std::map<std::string, MaterialStyle> styles;
        for (const auto& [name, style] : styles_)
        {
            if (savedStyles_.count(name) > 0 || style != DefaultStyle)
            {
                styles.emplace(name, style);
            }
        }

        MaterialStyleFileWriter::Write(styles, filePath_);
        savedStyles_ = std::move(styles);
        std::printf("Style saved: %s\n", filePath_.u8string().c_str());
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "그림체 파일을 저장하지 못했습니다: %s (%s)\n",
            filePath_.u8string().c_str(),
            exception.what());
        return false;
    }
}

MaterialStyle& MaterialStyleLibrary::FindOrAdd(const std::string& materialName)
{
    return styles_.emplace(materialName, DefaultStyle).first->second;
}

bool MaterialStyleLibrary::IsModified() const
{
    for (const auto& [name, style] : styles_)
    {
        if (style != FindSaved(name))
        {
            return true;
        }
    }
    return false;
}

const MaterialStyle& MaterialStyleLibrary::FindSaved(
    const std::string& materialName) const
{
    const auto style = savedStyles_.find(materialName);
    return style == savedStyles_.end() ? DefaultStyle : style->second;
}
