#include "FrameConstantBuffer.h"

#include "D3D12CommandContext.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

using Microsoft::WRL::ComPtr;

FrameConstantBuffer::FrameConstantBuffer(UINT dataSize)
    : dataSize_(dataSize),
      alignedSize_(
          (dataSize + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) &
          ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1))
{
}

FrameConstantBuffer::~FrameConstantBuffer()
{
    if (buffer_ != nullptr && mappedData_ != nullptr)
    {
        buffer_->Unmap(0, nullptr);
    }
}

bool FrameConstantBuffer::EnsureCapacity(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    std::size_t requiredCapacity)
{
    if (requiredCapacity <= capacity_)
    {
        return true;
    }

    const UINT64 bytesPerSlot =
        static_cast<UINT64>(alignedSize_) * D3D12Config::FrameCount;
    const std::size_t maximumCapacity =
        (std::numeric_limits<std::size_t>::max)() /
        static_cast<std::size_t>(bytesPerSlot);
    if (requiredCapacity > maximumCapacity)
    {
        std::fprintf(stderr, "Constant Buffer Capacity가 너무 큽니다.\n");
        return false;
    }

    std::size_t newCapacity = (std::max)(
        capacity_,
        static_cast<std::size_t>(1));
    while (newCapacity < requiredCapacity)
    {
        if (newCapacity > maximumCapacity / 2)
        {
            newCapacity = requiredCapacity;
            break;
        }

        newCapacity *= 2;
    }

    return Recreate(device, commandContext, newCapacity);
}

void FrameConstantBuffer::Write(
    UINT frameIndex,
    std::size_t slot,
    const void* data)
{
    std::memcpy(
        static_cast<unsigned char*>(mappedData_) +
            static_cast<std::size_t>(GetOffset(frameIndex, slot)),
        data,
        dataSize_);
}

D3D12_GPU_VIRTUAL_ADDRESS FrameConstantBuffer::GetGpuAddress(
    UINT frameIndex,
    std::size_t slot) const
{
    return buffer_->GetGPUVirtualAddress() + GetOffset(frameIndex, slot);
}

bool FrameConstantBuffer::Recreate(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    std::size_t newCapacity)
{
    // 기존 Resource를 교체하기 전에 Queue에 제출된 모든 참조가 끝났는지 Fence로 확인합니다.
    if (buffer_ != nullptr && !commandContext.WaitForGPU())
    {
        return false;
    }

    // CPU가 값을 직접 쓰고 GPU가 읽을 수 있도록 Upload Heap을 사용합니다.
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment = 0;
    // Frame별 구역의 크기를 Capacity로 고정하여 실행 중 칸 수가 바뀌어도 영역이 겹치지 않습니다.
    resourceDescription.Width =
        static_cast<UINT64>(alignedSize_) *
        D3D12Config::FrameCount *
        newCapacity;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> newBuffer;
    const HRESULT resourceResult = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&newBuffer));
    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "상수 버퍼용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    // 한 번 얻은 CPU 주소를 계속 보관하는 Persistent Mapping 방식으로 사용합니다.
    void* newMappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = newBuffer->Map(0, &readRange, &newMappedData);
    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "상수 버퍼의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memset(
        newMappedData,
        0,
        static_cast<SIZE_T>(resourceDescription.Width));

    if (buffer_ != nullptr && mappedData_ != nullptr)
    {
        buffer_->Unmap(0, nullptr);
    }

    buffer_ = std::move(newBuffer);
    mappedData_ = newMappedData;
    capacity_ = newCapacity;
    return true;
}

UINT64 FrameConstantBuffer::GetOffset(UINT frameIndex, std::size_t slot) const
{
    return (static_cast<UINT64>(frameIndex) * capacity_ + slot) * alignedSize_;
}
