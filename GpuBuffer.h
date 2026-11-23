#pragma once

#include <d3d12.h>
#include <wrl.h>

class D3D12CommandContext;

// GPU가 읽을 Buffer를 쓰임새에 맞는 Heap에 만듭니다.
class GpuBuffer final
{
public:
    GpuBuffer() = delete;

    // CPU가 값을 직접 쓰고 GPU가 읽는 Upload Heap Buffer를 만듭니다.
    static bool CreateUpload(
        ID3D12Device* device,
        UINT64 size,
        Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);

    // CPU 데이터를 Upload Heap에 쓴 뒤 GPU 전용 Default Heap Buffer로 복사하고 완료를 기다립니다.
    // 바뀌지 않는 Vertex/Index Buffer에 사용하며 finalState는 복사 뒤 전환할 사용 상태입니다.
    static bool CreateDefault(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const void* data,
        UINT64 size,
        D3D12_RESOURCE_STATES finalState,
        Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);

private:
    static bool Create(
        ID3D12Device* device,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_STATES initialState,
        UINT64 size,
        Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);
};
