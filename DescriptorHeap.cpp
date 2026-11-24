#include "DescriptorHeap.h"

#include <cstdio>

bool DescriptorHeap::Initialize(
    ID3D12Device* device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    UINT capacity,
    bool isShaderVisible)
{
    if (device == nullptr || capacity == 0)
    {
        std::fprintf(stderr, "DescriptorHeap::Initialize에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC description{};
    description.Type = type;
    description.NumDescriptors = capacity;
    description.Flags = isShaderVisible
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    description.NodeMask = 0;

    const HRESULT result = device->CreateDescriptorHeap(
        &description,
        IID_PPV_ARGS(&heap_));
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateDescriptorHeap 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    // 칸 사이의 간격은 GPU마다 다르므로 Device에 물어봅니다.
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(type);
    capacity_ = capacity;
    return true;
}

void DescriptorHeap::Reset()
{
    heap_.Reset();
    descriptorSize_ = 0;
    capacity_ = 0;
}

ID3D12DescriptorHeap* DescriptorHeap::Get() const
{
    return heap_.Get();
}

UINT DescriptorHeap::GetCapacity() const
{
    return capacity_;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(UINT slot) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        heap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(slot) * descriptorSize_;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT slot) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle =
        heap_->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(slot) * descriptorSize_;
    return handle;
}

UINT DescriptorHeap::GetSlot(D3D12_CPU_DESCRIPTOR_HANDLE handle) const
{
    const D3D12_CPU_DESCRIPTOR_HANDLE start =
        heap_->GetCPUDescriptorHandleForHeapStart();
    return static_cast<UINT>((handle.ptr - start.ptr) / descriptorSize_);
}
