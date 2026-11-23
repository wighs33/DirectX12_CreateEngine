#include "Texture.h"

#include "D3D12CommandContext.h"

#include <Windows.h>
#include <wincodec.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr UINT BytesPerPixel = 4;
    // 색 이미지는 sRGB로 저장되어 있으므로 Sample할 때 GPU가 선형 색으로 바꾸게 합니다.
    constexpr DXGI_FORMAT TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    constexpr UINT TextureMipLevels = 1;

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

    bool DecodeImageFile(
        const std::wstring& filePath,
        std::vector<std::uint8_t>& pixels,
        UINT& width,
        UINT& height)
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
                static_cast<UINT64>(decodedWidth) * BytesPerPixel;
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

            pixels = std::move(decodedPixels);
            width = decodedWidth;
            height = decodedHeight;
            return true;
        }();

        if (shouldUninitializeCOM)
        {
            CoUninitialize();
        }

        return decodeSucceeded;
    }
}

bool Texture::LoadFromFile(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const std::wstring& filePath)
{
    if (device == nullptr || filePath.empty())
    {
        std::fprintf(stderr, "Texture::LoadFromFile에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    std::vector<std::uint8_t> pixels;
    UINT decodedWidth = 0;
    UINT decodedHeight = 0;
    if (!DecodeImageFile(filePath, pixels, decodedWidth, decodedHeight))
    {
        return false;
    }

    return CreateFromPixels(
        device,
        commandContext,
        pixels,
        decodedWidth,
        decodedHeight);
}

bool Texture::CreateFromPixels(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const std::vector<std::uint8_t>& pixels,
    UINT width,
    UINT height)
{
    if (device == nullptr || width == 0 || height == 0 ||
        pixels.size() != static_cast<std::size_t>(width) * height * BytesPerPixel)
    {
        std::fprintf(stderr, "Texture::CreateFromPixels에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    D3D12_HEAP_PROPERTIES defaultHeapProperties{};
    defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultHeapProperties.CreationNodeMask = 1;
    defaultHeapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC textureDescription{};
    textureDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDescription.Alignment = 0;
    textureDescription.Width = width;
    textureDescription.Height = height;
    textureDescription.DepthOrArraySize = 1;
    textureDescription.MipLevels = TextureMipLevels;
    textureDescription.Format = TextureFormat;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> textureResource;
    HRESULT result = device->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDescription,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&textureResource));
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "Texture Resource 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    // 실제 이미지 크기에 맞는 Upload Row Pitch와 전체 Buffer 크기를 계산합니다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT uploadFootprint{};
    UINT uploadRowCount = 0;
    UINT64 uploadRowSize = 0;
    UINT64 uploadBufferSize = 0;
    device->GetCopyableFootprints(
        &textureDescription,
        0,
        1,
        0,
        &uploadFootprint,
        &uploadRowCount,
        &uploadRowSize,
        &uploadBufferSize);

    const UINT64 sourceRowSize =
        static_cast<UINT64>(width) * BytesPerPixel;
    if (uploadRowCount != height || uploadRowSize != sourceRowSize)
    {
        std::fprintf(stderr, "Texture Upload Footprint가 디코딩된 이미지 데이터와 일치하지 않습니다.\n");
        return false;
    }

    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeapProperties.CreationNodeMask = 1;
    uploadHeapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC uploadBufferDescription{};
    uploadBufferDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadBufferDescription.Alignment = 0;
    uploadBufferDescription.Width = uploadBufferSize;
    uploadBufferDescription.Height = 1;
    uploadBufferDescription.DepthOrArraySize = 1;
    uploadBufferDescription.MipLevels = 1;
    uploadBufferDescription.Format = DXGI_FORMAT_UNKNOWN;
    uploadBufferDescription.SampleDesc.Count = 1;
    uploadBufferDescription.SampleDesc.Quality = 0;
    uploadBufferDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadBufferDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> uploadBuffer;
    result = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "Texture Upload Buffer 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    void* mappedUploadData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    result = uploadBuffer->Map(0, &readRange, &mappedUploadData);
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "Texture Upload Buffer Map에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    auto* const destination =
        static_cast<std::uint8_t*>(mappedUploadData) + uploadFootprint.Offset;
    for (UINT row = 0; row < height; ++row)
    {
        std::memcpy(
            destination + static_cast<SIZE_T>(row) * uploadFootprint.Footprint.RowPitch,
            pixels.data() + static_cast<SIZE_T>(row) * sourceRowSize,
            static_cast<SIZE_T>(sourceRowSize));
    }

    const D3D12_RANGE writtenRange
    {
        static_cast<SIZE_T>(uploadFootprint.Offset),
        static_cast<SIZE_T>(uploadBufferSize)
    };
    uploadBuffer->Unmap(0, &writtenRange);

    if (!commandContext.BeginFrame(0))
    {
        return false;
    }

    ID3D12GraphicsCommandList* const commandList = commandContext.GetCommandList();

    D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
    destinationLocation.pResource = textureResource.Get();
    destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destinationLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
    sourceLocation.pResource = uploadBuffer.Get();
    sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    sourceLocation.PlacedFootprint = uploadFootprint;

    commandList->CopyTextureRegion(
        &destinationLocation,
        0,
        0,
        0,
        &sourceLocation,
        nullptr);

    D3D12_RESOURCE_BARRIER textureBarrier{};
    textureBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    textureBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    textureBarrier.Transition.pResource = textureResource.Get();
    textureBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    textureBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    textureBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &textureBarrier);

    if (!commandContext.Execute() || !commandContext.WaitForGPU())
    {
        return false;
    }

    // GPU가 Upload Buffer를 모두 사용한 뒤에만 최종 Resource와 파일 크기를 Texture가 소유합니다.
    textureResource_ = std::move(textureResource);
    width_ = width;
    height_ = height;
    return true;
}

void Texture::SetDescriptorSlot(UINT slot)
{
    descriptorSlot_ = slot;
}

UINT Texture::GetDescriptorSlot() const
{
    return descriptorSlot_;
}

ID3D12Resource* Texture::GetResource() const
{
    return textureResource_.Get();
}

UINT Texture::GetWidth() const
{
    return width_;
}

UINT Texture::GetHeight() const
{
    return height_;
}

DXGI_FORMAT Texture::GetFormat() const
{
    return TextureFormat;
}

UINT Texture::GetMipLevels() const
{
    return TextureMipLevels;
}
