#include "ResourceManager.h"

#include "D3D12CommandContext.h"

#include <cstdio>
#include <cwchar>
#include <utility>

bool ResourceManager::Initialize(
    ID3D12Device* device,
    D3D12CommandContext& commandContext)
{
    if (device == nullptr)
    {
        std::fprintf(stderr, "ResourceManager::Initialize에 유효하지 않은 Device가 전달되었습니다.\n");
        return false;
    }

    device_ = device;
    commandContext_ = &commandContext;
    return true;
}

Texture* ResourceManager::LoadTexture(const std::wstring& filePath)
{
    if (device_ == nullptr || commandContext_ == nullptr)
    {
        std::fprintf(stderr, "Texture를 로드하기 전에 ResourceManager를 초기화해야 합니다.\n");
        return nullptr;
    }

    if (filePath.empty())
    {
        std::fprintf(stderr, "ResourceManager::LoadTexture에 빈 파일 경로가 전달되었습니다.\n");
        return nullptr;
    }

    const auto cachedTexture = textures_.find(filePath);
    if (cachedTexture != textures_.end())
    {
        return cachedTexture->second.get();
    }

    auto texture = std::make_unique<Texture>();
    if (!texture->LoadFromFile(device_, *commandContext_, filePath))
    {
        // 실패한 Texture는 Cache에 넣지 않으므로 다음 요청에서 다시 시도할 수 있습니다.
        return nullptr;
    }

    Texture* const texturePointer = texture.get();
    textures_.emplace(filePath, std::move(texture));

    // Cache Miss에서 실제 파일과 GPU Resource를 새로 만든 경우에만 한 번 출력합니다.
    std::wprintf(L"Texture loaded: %ls\n", filePath.c_str());
    return texturePointer;
}

Texture* ResourceManager::GetTexture(const std::wstring& filePath)
{
    const auto cachedTexture = textures_.find(filePath);
    if (cachedTexture == textures_.end())
    {
        return nullptr;
    }

    return cachedTexture->second.get();
}

void ResourceManager::Clear()
{
    textures_.clear();
}
