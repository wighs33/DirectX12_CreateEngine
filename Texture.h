#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <string>
#include <vector>

class D3D12CommandContext;

// 이미지 파일을 RGBA8 Pixel로 디코딩하고 GPU Texture Resource로 업로드합니다.
class Texture final
{
public:
    Texture() = default;
    ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool LoadFromFile(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const std::wstring& filePath);

    // RGBA8 Pixel로 바로 Texture를 만듭니다.
    bool CreateFromPixels(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const std::vector<std::uint8_t>& pixels,
        UINT width,
        UINT height);

    // Shader Visible Heap에서 이 Texture의 SRV가 있는 칸입니다.
    void SetDescriptorSlot(UINT slot);
    UINT GetDescriptorSlot() const;

    ID3D12Resource* GetResource() const;
    UINT GetWidth() const;
    UINT GetHeight() const;
    DXGI_FORMAT GetFormat() const;
    UINT GetMipLevels() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    UINT width_ = 0;
    UINT height_ = 0;
    UINT descriptorSlot_ = 0;
};
