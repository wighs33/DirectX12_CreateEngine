#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <string>

class D3D12CommandContext;
struct ImageData;

// RGBA8 이미지를 GPU Texture Resource로 업로드합니다.
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

    // 이미 풀어 둔 이미지로 Texture를 만듭니다.
    bool CreateFromImage(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const ImageData& image);

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
