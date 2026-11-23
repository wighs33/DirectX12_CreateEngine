#include "GpuBuffer.h"

#include "D3D12CommandContext.h"

#include <cstdio>
#include <cstring>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace
{
    bool CreateBuffer(
        ID3D12Device* device,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_STATES initialState,
        UINT64 size,
        ComPtr<ID3D12Resource>& buffer)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = heapType;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resourceDescription{};
        resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDescription.Alignment = 0;
        resourceDescription.Width = size;
        resourceDescription.Height = 1;
        resourceDescription.DepthOrArraySize = 1;
        resourceDescription.MipLevels = 1;
        resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
        resourceDescription.SampleDesc.Count = 1;
        resourceDescription.SampleDesc.Quality = 0;
        resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

        const HRESULT result = device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescription,
            initialState,
            nullptr,
            IID_PPV_ARGS(&buffer));
        if (FAILED(result))
        {
            std::fprintf(
                stderr,
                "Buffer용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
                static_cast<unsigned int>(result));
            return false;
        }

        return true;
    }
}

bool CreateDefaultBuffer(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const void* data,
    UINT64 size,
    D3D12_RESOURCE_STATES finalState,
    ComPtr<ID3D12Resource>& buffer)
{
    if (device == nullptr || data == nullptr || size == 0)
    {
        std::fprintf(stderr, "CreateDefaultBuffer에 유효하지 않은 인자가 전달되었습니다.\n");
        return false;
    }

    // [1단계] CPU가 쓸 수 있는 Upload Heap Buffer에 데이터를 복사합니다.
    ComPtr<ID3D12Resource> uploadBuffer;
    if (!CreateBuffer(
            device,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            size,
            uploadBuffer))
    {
        return false;
    }

    void* mappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = uploadBuffer->Map(0, &readRange, &mappedData);
    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "Upload Buffer의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memcpy(mappedData, data, static_cast<std::size_t>(size));
    const D3D12_RANGE writtenRange{ 0, static_cast<SIZE_T>(size) };
    uploadBuffer->Unmap(0, &writtenRange);

    // [2단계] GPU만 읽는 Default Heap Buffer를 만듭니다.
    // Buffer는 항상 COMMON으로 생성되고 복사 명령에서 COPY_DEST로 자동 전환됩니다.
    ComPtr<ID3D12Resource> defaultBuffer;
    if (!CreateBuffer(
            device,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_COMMON,
            size,
            defaultBuffer))
    {
        return false;
    }

    // [3단계] GPU에게 복사를 시키고 실제 사용할 상태로 전환합니다.
    if (!commandContext.BeginFrame(0))
    {
        return false;
    }

    ID3D12GraphicsCommandList* const commandList = commandContext.GetCommandList();
    commandList->CopyBufferRegion(
        defaultBuffer.Get(),
        0,
        uploadBuffer.Get(),
        0,
        size);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = defaultBuffer.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = finalState;
    commandList->ResourceBarrier(1, &barrier);

    // GPU가 Upload Buffer를 모두 읽은 뒤에만 함수를 나가며 Upload Buffer를 해제합니다.
    if (!commandContext.Execute() || !commandContext.WaitForGPU())
    {
        return false;
    }

    buffer = std::move(defaultBuffer);
    return true;
}
