#pragma once

#include "MaterialStyleLibrary.h"
#include "Model.h"
#include "ModelData.h"
#include "Texture.h"
#include "TextureHeap.h"

#include <d3d12.h>

#include <cstddef>
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

    // 흰색 Texture와 기본 도형까지 만들어 둡니다.
    bool Initialize(
        ID3D12Device* device,
        D3D12CommandContext& commandContext);

    Texture* LoadTexture(const std::wstring& filePath);
    Texture* GetTexture(const std::wstring& filePath);

    // 외곽선 방향과 재질을 만들어 붙이고 모델을 이름으로 등록합니다.
    Model* AddModel(const std::string& id, ModelData modelData);

    // glTF 파일을 한 번만 읽고 같은 경로를 다시 요청하면 이미 만든 모델을 돌려줍니다.
    Model* LoadModel(const std::filesystem::path& filePath);

    // 이미 등록하거나 읽은 모델이 없으면 nullptr을 반환합니다.
    Model* FindModel(const std::string& id);

    // 모든 Texture의 SRV가 들어 있는 Heap입니다.
    TextureHeap& GetTextureHeap();
    const TextureHeap& GetTextureHeap() const;

    // 지금까지 만든 재질의 수이며 재질 상수 버퍼에 필요한 칸 수입니다.
    std::size_t GetMaterialCount() const;

    void Clear();

private:
    // Device와 CommandContext의 실제 수명은 D3D12Context가 관리하며 여기서는 소유하지 않습니다.
    ID3D12Device* device_ = nullptr;
    D3D12CommandContext* commandContext_ = nullptr;

    static constexpr UINT TextureHeapCapacity = 256;

    bool CreateWhiteTexture();
    bool CreatePrimitiveModels();
    // Texture가 없거나 읽지 못한 재질은 흰색 Texture를 써서 정점 색만 보이게 합니다.
    const Texture& GetMaterialTexture(const ModelMaterialData& material);

    TextureHeap textureHeap_;
    Texture whiteTexture_;
    MaterialStyleLibrary styles_;
    std::size_t materialCount_ = 0;
    std::unordered_map<std::wstring, std::unique_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};
