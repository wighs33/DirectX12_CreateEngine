#include "TextureHeap.h"

#include "Texture.h"

#include <cstdio>

bool TextureHeap::Initialize(ID3D12Device* device, UINT capacity)
{
    if (device == nullptr || capacity == 0)
    {
        std::fprintf(stderr, "TextureHeap::Initialize에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescription.NumDescriptors = capacity;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDescription.NodeMask = 0;

    const HRESULT result = device->CreateDescriptorHeap(
        &heapDescription,
        IID_PPV_ARGS(&heap_));
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "SRV Descriptor Heap 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    // 칸 사이의 간격은 GPU마다 다르므로 Device에 물어봅니다.
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    capacity_ = capacity;
    count_ = 0;
    return true;
}

bool TextureHeap::Add(
    ID3D12Device* device,
    const Texture& texture,
    UINT& slot)
{
    if (device == nullptr || heap_ == nullptr || count_ >= capacity_)
    {
        std::fprintf(
            stderr,
            "Texture를 연결할 빈 Descriptor 칸이 없습니다. (최대 %u칸)\n",
            capacity_);
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription{};
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescription.Format = texture.GetFormat();
    srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Texture2D.MostDetailedMip = 0;
    srvDescription.Texture2D.MipLevels = texture.GetMipLevels();
    srvDescription.Texture2D.PlaneSlice = 0;
    srvDescription.Texture2D.ResourceMinLODClamp = 0.0f;

    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        heap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(count_) * descriptorSize_;
    device->CreateShaderResourceView(
        texture.GetResource(),
        &srvDescription,
        handle);

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
    D3D12_GPU_DESCRIPTOR_HANDLE handle =
        heap_->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(slot) * descriptorSize_;
    return handle;
}
