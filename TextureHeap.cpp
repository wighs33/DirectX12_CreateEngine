#include "TextureHeap.h"

#include "Texture.h"

#include <cstdio>

bool TextureHeap::Initialize(ID3D12Device* device, UINT capacity)
{
    count_ = 0;
    return heap_.Initialize(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        capacity,
        true);
}

bool TextureHeap::Add(
    ID3D12Device* device,
    const Texture& texture,
    UINT& slot)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription{};
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescription.Format = texture.GetFormat();
    srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Texture2D.MostDetailedMip = 0;
    srvDescription.Texture2D.MipLevels = texture.GetMipLevels();
    srvDescription.Texture2D.PlaneSlice = 0;
    srvDescription.Texture2D.ResourceMinLODClamp = 0.0f;
    return Add(device, texture.GetResource(), srvDescription, slot);
}

bool TextureHeap::Add(
    ID3D12Device* device,
    ID3D12Resource* resource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC& description,
    UINT& slot)
{
    if (device == nullptr || count_ >= heap_.GetCapacity())
    {
        std::fprintf(
            stderr,
            "Texture를 연결할 빈 Descriptor 칸이 없습니다. (최대 %u칸)\n",
            heap_.GetCapacity());
        return false;
    }

    device->CreateShaderResourceView(
        resource,
        &description,
        heap_.GetCpuHandle(count_));

    slot = count_;
    ++count_;
    return true;
}

ID3D12DescriptorHeap* TextureHeap::GetHeap() const
{
    return heap_.Get();
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureHeap::GetGpuHandle(UINT slot) const
{
    return heap_.GetGpuHandle(slot);
}
