#include "TextureUploader.h"

#include "D3D12CommandContext.h"
#include "GpuBarrier.h"
#include "GpuBuffer.h"
#include "ImageLoader.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

using Microsoft::WRL::ComPtr;

bool TextureUploader::Upload(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const ImageData& image,
    ID3D12Resource* texture)
{
    // 실제 이미지 크기에 맞는 Upload Row Pitch와 전체 Buffer 크기를 계산합니다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT uploadFootprint{};
    UINT uploadRowCount = 0;
    UINT64 uploadRowSize = 0;
    UINT64 uploadBufferSize = 0;
    const D3D12_RESOURCE_DESC textureDescription = texture->GetDesc();
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
        static_cast<UINT64>(image.width) * ImageData::BytesPerPixel;
    if (uploadRowCount != image.height || uploadRowSize != sourceRowSize)
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
    const HRESULT result = uploadBuffer->Map(0, &readRange, &mappedUploadData);
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
    for (UINT row = 0; row < image.height; ++row)
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
    destinationLocation.pResource = texture;
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

    GpuBarrier::Transition(
        commandList,
        texture,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // GPU가 Upload Buffer를 모두 읽은 뒤에만 함수를 나가며 Upload Buffer를 해제합니다.
    return commandContext.Execute() && commandContext.WaitForGPU();
}
