#include "ImageLoader.h"

#include <wincodec.h>
#include <wrl.h>

#include <cstddef>
#include <cstdio>
#include <limits>
#include <utility>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

namespace
{
    std::string ToUTF8(const std::wstring& text)
    {
        if (text.empty())
        {
            return {};
        }

        if (text.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        {
            return "<경로가 너무 깁니다>";
        }

        const int characterCount = static_cast<int>(text.size());
        const int utf8Size = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            text.data(),
            characterCount,
            nullptr,
            0,
            nullptr,
            nullptr);
        if (utf8Size == 0)
        {
            return "<경로 변환 실패>";
        }

        std::string utf8Text(static_cast<std::size_t>(utf8Size), '\0');
        if (WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                text.data(),
                characterCount,
                &utf8Text[0],
                utf8Size,
                nullptr,
                nullptr) == 0)
        {
            return "<경로 변환 실패>";
        }

        return utf8Text;
    }

    void PrintWICError(
        const char* operation,
        const std::wstring& filePath,
        HRESULT result)
    {
        const std::string utf8FilePath = ToUTF8(filePath);
        std::fprintf(
            stderr,
            "Texture 파일 처리에 실패했습니다. 경로: %s, 작업: %s, HRESULT: 0x%08X\n",
            utf8FilePath.c_str(),
            operation,
            static_cast<unsigned int>(result));
    }
}

bool ImageLoader::Load(const std::wstring& filePath, ImageData& image)
{
    // WIC는 COM Interface이므로 현재 Thread에서 Decode 작업 동안 COM을 초기화합니다.
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
    {
        PrintWICError("CoInitializeEx", filePath, comResult);
        return false;
    }

    const bool shouldUninitializeCOM = SUCCEEDED(comResult);

    // Lambda 안의 WIC ComPtr가 모두 파괴된 다음 CoUninitialize를 호출합니다.
    const bool decodeSucceeded = [&]() -> bool
    {
        ComPtr<IWICImagingFactory> imagingFactory;
        HRESULT result = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&imagingFactory));
        if (FAILED(result))
        {
            PrintWICError("IWICImagingFactory 생성", filePath, result);
            return false;
        }

        // WIC가 파일 내용을 확인하여 PNG와 JPG를 같은 코드 경로로 Decoder에 연결합니다.
        ComPtr<IWICBitmapDecoder> decoder;
        result = imagingFactory->CreateDecoderFromFilename(
            filePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder);
        if (FAILED(result))
        {
            PrintWICError("이미지 Decoder 생성", filePath, result);
            return false;
        }

        ComPtr<IWICBitmapFrameDecode> frame;
        result = decoder->GetFrame(0, &frame);
        if (FAILED(result))
        {
            PrintWICError("첫 번째 이미지 Frame 읽기", filePath, result);
            return false;
        }

        UINT decodedWidth = 0;
        UINT decodedHeight = 0;
        result = frame->GetSize(&decodedWidth, &decodedHeight);
        if (FAILED(result) || decodedWidth == 0 || decodedHeight == 0)
        {
            PrintWICError("이미지 크기 읽기", filePath, result);
            return false;
        }

        WICPixelFormatGUID sourcePixelFormat{};
        result = frame->GetPixelFormat(&sourcePixelFormat);
        if (FAILED(result))
        {
            PrintWICError("원본 Pixel Format 읽기", filePath, result);
            return false;
        }

        ComPtr<IWICFormatConverter> formatConverter;
        result = imagingFactory->CreateFormatConverter(&formatConverter);
        if (FAILED(result))
        {
            PrintWICError("IWICFormatConverter 생성", filePath, result);
            return false;
        }

        BOOL canConvert = FALSE;
        result = formatConverter->CanConvert(
            sourcePixelFormat,
            GUID_WICPixelFormat32bppRGBA,
            &canConvert);
        if (FAILED(result) || canConvert == FALSE)
        {
            PrintWICError("RGBA8 Pixel Format 변환 확인", filePath, result);
            return false;
        }

        // WIC의 32bpp RGBA 메모리 순서는 R8G8B8A8 형식과 일치합니다.
        result = formatConverter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);
        if (FAILED(result))
        {
            PrintWICError("RGBA8 Pixel Format 변환", filePath, result);
            return false;
        }

        const UINT64 rowSize =
            static_cast<UINT64>(decodedWidth) * ImageData::BytesPerPixel;
        const UINT64 pixelBufferSize =
            rowSize * decodedHeight;
        if (rowSize > (std::numeric_limits<UINT>::max)() ||
            pixelBufferSize > (std::numeric_limits<UINT>::max)())
        {
            const std::string utf8FilePath = ToUTF8(filePath);
            std::fprintf(
                stderr,
                "Texture 이미지가 현재 CPU Pixel Buffer 범위를 초과합니다. 경로: %s\n",
                utf8FilePath.c_str());
            return false;
        }

        std::vector<std::uint8_t> decodedPixels(
            static_cast<std::size_t>(pixelBufferSize));
        result = formatConverter->CopyPixels(
            nullptr,
            static_cast<UINT>(rowSize),
            static_cast<UINT>(pixelBufferSize),
            decodedPixels.data());
        if (FAILED(result))
        {
            PrintWICError("RGBA8 Pixel 복사", filePath, result);
            return false;
        }

        image.pixels = std::move(decodedPixels);
        image.width = decodedWidth;
        image.height = decodedHeight;
        return true;
    }();

    if (shouldUninitializeCOM)
    {
        CoUninitialize();
    }

    return decodeSucceeded;
}
