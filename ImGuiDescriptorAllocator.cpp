#include "ImGuiDescriptorAllocator.h"

bool ImGuiDescriptorAllocator::Initialize(ID3D12Device* device)
{
    if (!heap_.Initialize(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            Capacity,
            true))
    {
        return false;
    }

    // 뒤에서부터 꺼내므로 0번 칸을 가장 먼저 빌려 주도록 거꾸로 담습니다.
    freeSlots_.clear();
    freeSlots_.reserve(Capacity);
    for (UINT slot = Capacity; slot > 0; --slot)
    {
        freeSlots_.push_back(slot - 1);
    }
    return true;
}

void ImGuiDescriptorAllocator::Reset()
{
    freeSlots_.clear();
    heap_.Reset();
}

bool ImGuiDescriptorAllocator::Allocate(
    D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
{
    if (freeSlots_.empty())
    {
        return false;
    }

    const UINT slot = freeSlots_.back();
    freeSlots_.pop_back();
    cpuHandle = heap_.GetCpuHandle(slot);
    gpuHandle = heap_.GetGpuHandle(slot);
    return true;
}

bool ImGuiDescriptorAllocator::Free(
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    if (heap_.Get() == nullptr)
    {
        return false;
    }

    const UINT slot = heap_.GetSlot(cpuHandle);
    if (slot >= Capacity || heap_.GetGpuHandle(slot).ptr != gpuHandle.ptr)
    {
        return false;
    }

    freeSlots_.push_back(slot);
    return true;
}

ID3D12DescriptorHeap* ImGuiDescriptorAllocator::GetHeap() const
{
    return heap_.Get();
}
