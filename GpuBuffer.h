#pragma once

#include <d3d12.h>
#include <wrl.h>

class D3D12CommandContext;

// CPU 데이터를 Upload Heap에 쓴 뒤 GPU 전용 Default Heap Buffer로 복사하고 완료를 기다립니다.
// 바뀌지 않는 Vertex/Index Buffer에 사용하며 finalState는 복사 뒤 전환할 사용 상태입니다.
bool CreateDefaultBuffer(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const void* data,
    UINT64 size,
    D3D12_RESOURCE_STATES finalState,
    Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);
