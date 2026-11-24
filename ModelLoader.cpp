#include "ModelLoader.h"

#include "GltfModelReader.h"

#include <cstdio>
#include <exception>

bool ModelLoader::LoadGltf(
    const std::filesystem::path& filePath,
    ModelData& modelData)
{
    try
    {
        modelData = GltfModelReader(filePath).Read();
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "glTF 파일을 읽지 못했습니다: %s (%s)\n",
            filePath.u8string().c_str(),
            exception.what());
        return false;
    }
}
