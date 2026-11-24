#include "Texture.h"

#include "GpuTexture.h"
#include "ImageLoader.h"
#include "TextureUploader.h"

#include <cstddef>
#include <cstdio>
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
    if (device == nullptr || image.width == 0 || image.height == 0 ||
        image.pixels.size() !=
            static_cast<std::size_t>(image.width) * image.height *
                ImageData::BytesPerPixel)
    {
        std::fprintf(stderr, "Texture::CreateFromImage에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    GpuTextureDescription description;
    description.width = image.width;
    description.height = image.height;
    description.format = TextureFormat;
    description.initialState = D3D12_RESOURCE_STATE_COPY_DEST;

    ComPtr<ID3D12Resource> textureResource;
    if (!GpuTexture::Create(device, description, textureResource) ||
        !TextureUploader::Upload(
            device,
            commandContext,
            image,
            textureResource.Get()))
    {
        return false;
    }

    // 픽셀을 모두 옮긴 뒤에만 최종 Resource와 크기를 Texture가 소유합니다.
    textureResource_ = std::move(textureResource);
    width_ = image.width;
    height_ = image.height;
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
