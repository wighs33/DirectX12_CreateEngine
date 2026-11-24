#pragma once

#include "DescriptorHeap.h"

#include <d3d12.h>

#include <vector>

// ImGui가 글꼴 같은 그림을 연결할 칸을 빌려 주고 돌려받습니다.
class ImGuiDescriptorAllocator final
{
public:
    ImGuiDescriptorAllocator() = default;
    ~ImGuiDescriptorAllocator() = default;

    ImGuiDescriptorAllocator(const ImGuiDescriptorAllocator&) = delete;
    ImGuiDescriptorAllocator& operator=(const ImGuiDescriptorAllocator&) = delete;

    bool Initialize(ID3D12Device* device);
    void Reset();

    // 빈 칸이 없으면 false를 반환합니다.
    bool Allocate(
        D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle);

    // 빌려 준 칸이 아니면 false를 반환합니다.
    bool Free(
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    ID3D12DescriptorHeap* GetHeap() const;

private:
    static constexpr UINT Capacity = 16;

    DescriptorHeap heap_;
    // 아직 빌려 주지 않은 칸의 번호입니다.
    std::vector<UINT> freeSlots_;
};
