#pragma once

#include "Texture.h"

#include <d3d12.h>

#include <memory>
#include <string>
#include <unordered_map>

class D3D12CommandContext;

// 파일 경로를 Key로 Texture를 한 번만 생성하고 수명을 소유하는 최소 Resource Cache입니다.
class ResourceManager final
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    bool Initialize(
        ID3D12Device* device,
        D3D12CommandContext& commandContext);

    Texture* LoadTexture(const std::wstring& filePath);
    Texture* GetTexture(const std::wstring& filePath);
    void Clear();

private:
    // Device와 CommandContext의 실제 수명은 D3D12Context가 관리하며 여기서는 소유하지 않습니다.
    ID3D12Device* device_ = nullptr;
    D3D12CommandContext* commandContext_ = nullptr;

    std::unordered_map<std::wstring, std::unique_ptr<Texture>> textures_;
};
