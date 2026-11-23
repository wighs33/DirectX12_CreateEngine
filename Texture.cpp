#include "Texture.h"

#include "D3D12CommandContext.h"
#include "GpuBuffer.h"
#include "ImageLoader.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace
{
    // 색 이미지는 sRGB로 저장되어 있으므로 Sample할 때 GPU가 선형 색으로 바꾸게 합니다.
    constexpr DXGI_FORMAT TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    constexpr UINT TextureMipLevels = 1;
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

    ImageData image;
    return ImageLoader::Load(filePath, image) &&
        CreateFromImage(device, commandContext, image);
}

bool Texture::CreateFromImage(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const ImageData& image)
{
    const UINT width = image.width;
    const UINT height = image.height;
    if (device == nullptr || width == 0 || height == 0 ||
        image.pixels.size() !=
            static_cast<std::size_t>(width) * height * ImageData::BytesPerPixel)
    {
        std::fprintf(stderr, "Texture::CreateFromImage에 유효하지 않은 인자가 전달되었습니다.\n");
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
        static_cast<UINT64>(width) * ImageData::BytesPerPixel;
    if (uploadRowCount != height || uploadRowSize != sourceRowSize)
    {
        std::fprintf(stderr, "Texture Upload Footprint가 디코딩된 이미지 데이터와 일치하지 않습니다.\n");
        return false;
    }

    ComPtr<ID3D12Resource> uploadBuffer;
    if (!GpuBuffer::CreateUpload(device, uploadBufferSize, uploadBuffer))
    {
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
            image.pixels.data() + static_cast<SIZE_T>(row) * sourceRowSize,
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
