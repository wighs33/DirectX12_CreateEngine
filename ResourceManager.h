#pragma once

#include "Model.h"
#include "ModelData.h"
#include "Texture.h"

#include <d3d12.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class D3D12CommandContext;

// Texture와 Model을 이름이나 경로로 한 번만 생성하고 수명을 소유하는 최소 Resource Cache입니다.
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

    // 코드로 만든 모델을 이름으로 등록합니다.
    Model* AddModel(const std::string& id, const ModelData& modelData);

    // glTF 파일을 한 번만 읽고 같은 경로를 다시 요청하면 이미 만든 모델을 돌려줍니다.
    Model* LoadModel(const std::filesystem::path& filePath);

    // 이미 등록하거나 읽은 모델이 없으면 nullptr을 반환합니다.
    Model* FindModel(const std::string& id);
    void Clear();

private:
    // Device와 CommandContext의 실제 수명은 D3D12Context가 관리하며 여기서는 소유하지 않습니다.
    ID3D12Device* device_ = nullptr;
    D3D12CommandContext* commandContext_ = nullptr;

    std::unordered_map<std::wstring, std::unique_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};
