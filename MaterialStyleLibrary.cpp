#include "MaterialStyleLibrary.h"

#include "MaterialStyleFileReader.h"
#include "MaterialStyleFileWriter.h"

#include <cstdio>
#include <exception>
#include <system_error>
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
        canSave_ = true;
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "그림체 파일을 읽지 못했습니다: %s (%s)\n",
            filePath_.u8string().c_str(),
            exception.what());

        std::error_code existenceError;
        canSave_ = !std::filesystem::exists(filePath_, existenceError);
        return false;
    }
}

bool MaterialStyleLibrary::Save()
{
    if (!CanSave())
    {
        std::fprintf(
            stderr,
            "읽지 못한 그림체 파일은 덮어쓰지 않습니다. 파일을 고친 뒤 Reload하세요: %s\n",
            filePath_.u8string().c_str());
        return false;
    }

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

bool MaterialStyleLibrary::CanSave() const
{
    return canSave_;
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
