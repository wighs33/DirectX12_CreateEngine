#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <string>

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

    ID3D12Resource* GetResource() const;
    UINT GetWidth() const;
    UINT GetHeight() const;
    DXGI_FORMAT GetFormat() const;
    UINT GetMipLevels() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    UINT width_ = 0;
    UINT height_ = 0;
};
