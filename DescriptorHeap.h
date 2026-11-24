#pragma once

#include <d3d12.h>
#include <wrl.h>

// 자원을 어떻게 볼지 적은 Descriptor를 칸칸이 담아 둡니다.
class DescriptorHeap final
{
public:
    DescriptorHeap() = default;
    ~DescriptorHeap() = default;

    DescriptorHeap(const DescriptorHeap&) = delete;
    DescriptorHeap& operator=(const DescriptorHeap&) = delete;

    // isShaderVisible이면 Shader가 칸을 직접 읽을 수 있습니다.
    bool Initialize(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        UINT capacity,
        bool isShaderVisible);

    void Reset();

    ID3D12DescriptorHeap* Get() const;
    UINT GetCapacity() const;

    // View를 만들거나 그릴 대상으로 연결할 때 쓰는 칸의 주소입니다.
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT slot) const;

    // Shader가 읽을 수 있는 Heap에서 Descriptor Table에 연결할 칸의 주소입니다.
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT slot) const;

    // 주소가 몇 번째 칸인지 돌려줍니다.
    UINT GetSlot(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    UINT descriptorSize_ = 0;
    UINT capacity_ = 0;
};
