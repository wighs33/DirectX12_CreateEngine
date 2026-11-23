#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

// 파일에서 푼 이미지입니다. 픽셀은 왼쪽 위부터 RGBA 순서로 1 Byte씩 이어집니다.
struct ImageData
{
    static constexpr UINT BytesPerPixel = 4;

    UINT width = 0;
    UINT height = 0;
    std::vector<std::uint8_t> pixels;
};

// PNG나 JPG 파일을 RGBA8 Pixel로 풉니다. GPU는 다루지 않습니다.
class ImageLoader final
{
public:
    ImageLoader() = delete;

    // 실패하면 이유를 출력하고 image를 바꾸지 않습니다.
    static bool Load(const std::wstring& filePath, ImageData& image);
};
