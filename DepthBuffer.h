#pragma once

#include <d3d12.h>
#include <wrl.h>

// 앞뒤를 비교할 깊이 값을 보관합니다.
class DepthBuffer final
{
public:
    DepthBuffer() = default;
    ~DepthBuffer() = default;

    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer& operator=(const DepthBuffer&) = delete;

    // sampleCount는 한 픽셀에 담을 깊이의 수이고 initialState는 처음 쓰일 때의 상태입니다.
    bool Initialize(
        ID3D12Device* device,
        UINT width,
        UINT height,
        UINT sampleCount,
        D3D12_RESOURCE_STATES initialState);

    // 이전 Frame의 깊이가 남지 않도록 전체를 가장 먼 값으로 지웁니다.
    void Clear(ID3D12GraphicsCommandList* commandList) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
    ID3D12Resource* GetResource() const;

private:
    // 지울 때 쓰는 가장 먼 깊이입니다.
    static constexpr float FarthestDepth = 1.0f;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
};
