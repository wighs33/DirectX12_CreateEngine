#include "GpuTexture.h"

#include <cstdio>

bool GpuTexture::Create(
    ID3D12Device* device,
    const GpuTextureDescription& description,
    Microsoft::WRL::ComPtr<ID3D12Resource>& texture)
{
    if (device == nullptr || description.width == 0 || description.height == 0)
    {
        std::fprintf(stderr, "GpuTexture::Create에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Alignment = 0;
    resourceDescription.Width = description.width;
    resourceDescription.Height = description.height;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = description.format;
    resourceDescription.SampleDesc.Count = description.sampleCount;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescription.Flags = description.flags;

    const HRESULT result = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        description.initialState,
        description.clearValue,
        IID_PPV_ARGS(&texture));
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "Texture용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    return true;
}
