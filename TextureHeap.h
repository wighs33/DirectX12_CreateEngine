#pragma once

#include <d3d12.h>
#include <wrl.h>

class Texture;

// 모든 Texture의 SRV를 여러 칸에 나눠 담는 Shader Visible Descriptor Heap입니다.
class TextureHeap final
{
public:
    TextureHeap() = default;
    ~TextureHeap() = default;

    TextureHeap(const TextureHeap&) = delete;
    TextureHeap& operator=(const TextureHeap&) = delete;

    bool Initialize(ID3D12Device* device, UINT capacity);

    // 빈 칸에 SRV를 만들고 그 칸 번호를 돌려줍니다.
    bool Add(ID3D12Device* device, const Texture& texture, UINT& slot);

    ID3D12DescriptorHeap* GetHeap() const;

    // Root Signature의 Descriptor Table에 연결할 칸의 GPU 주소입니다.
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT slot) const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    UINT descriptorSize_ = 0;
    UINT capacity_ = 0;
    UINT count_ = 0;
};
