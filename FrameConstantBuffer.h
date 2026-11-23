#pragma once

#include "D3D12Config.h"

#include <d3d12.h>
#include <wrl.h>

#include <cstddef>

class D3D12CommandContext;

// 같은 크기의 상수 값을 여러 칸에 담고 Frame마다 구역을 나눠 쓰는 Upload Heap Buffer입니다.
// GPU가 이전 Frame의 값을 읽는 동안에도 이번 Frame의 구역에 안전하게 쓸 수 있습니다.
class FrameConstantBuffer final
{
public:
    FrameConstantBuffer() = default;
    ~FrameConstantBuffer();

    FrameConstantBuffer(const FrameConstantBuffer&) = delete;
    FrameConstantBuffer& operator=(const FrameConstantBuffer&) = delete;

    // dataSize는 한 칸에 담을 구조체의 크기이며 실제 칸은 256 Byte 단위로 정렬됩니다.
    explicit FrameConstantBuffer(UINT dataSize);

    // 필요한 칸이 모자랄 때만 두 배씩 늘립니다. 교체 전에 GPU가 이전 Buffer를 다 쓸 때까지 기다립니다.
    bool EnsureCapacity(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        std::size_t requiredCapacity);

    void Write(UINT frameIndex, std::size_t slot, const void* data);

    // Root CBV에 연결할 칸의 GPU 주소입니다.
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(
        UINT frameIndex,
        std::size_t slot) const;

private:
    bool Recreate(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        std::size_t newCapacity);
    UINT64 GetOffset(UINT frameIndex, std::size_t slot) const;

    UINT dataSize_ = 0;
    UINT alignedSize_ = 0;
    std::size_t capacity_ = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
    // 한 번 Map한 뒤 계속 사용하는 CPU 주소입니다.
    void* mappedData_ = nullptr;
};
