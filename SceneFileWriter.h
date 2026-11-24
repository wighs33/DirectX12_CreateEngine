#pragma once

#include <filesystem>

struct SceneData;

// 장면의 내용을 사람이 읽을 수 있는 JSON File로 적습니다.
class SceneFileWriter final
{
public:
    SceneFileWriter() = delete;

    // 폴더가 없으면 만들고, 실패하면 이유를 담은 예외를 던집니다.
    static void Write(
        const SceneData& data,
        const std::filesystem::path& filePath);
};
