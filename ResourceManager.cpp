#include "ResourceManager.h"

#include "D3D12CommandContext.h"
#include "ImageLoader.h"
#include "ModelLoader.h"
#include "OutlineNormalBaker.h"
#include "PrimitiveGeometry.h"

#include <cstdio>
#include <filesystem>
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

    // 그림체 파일이 없어도 모든 재질이 기본 그림체로 그려집니다.
    styles_.Load(std::filesystem::path("Assets") / "Styles" / "PandoraStyle.json");

    return textureHeap_.Initialize(device, TextureHeapCapacity) &&
        CreateWhiteTexture() &&
        CreatePrimitiveModels();
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

    UINT slot = 0;
    if (!textureHeap_.Add(device_, *texture, slot))
    {
        return nullptr;
    }
    texture->SetDescriptorSlot(slot);

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

Model* ResourceManager::AddModel(
    const std::string& id,
    ModelData modelData)
{
    if (device_ == nullptr || commandContext_ == nullptr)
    {
        std::fprintf(stderr, "Model을 만들기 전에 ResourceManager를 초기화해야 합니다.\n");
        return nullptr;
    }

    OutlineNormalBaker::Bake(modelData);

    std::vector<Material> materials;
    for (const ModelMaterialData& materialData : modelData.materials)
    {
        materials.emplace_back(
            materialData.name,
            GetMaterialTexture(materialData),
            styles_.FindOrAdd(materialData.name),
            materialCount_ + materials.size());
    }
    const std::size_t addedMaterialCount = materials.size();

    auto model = std::make_unique<Model>();
    if (!model->Initialize(
            device_,
            *commandContext_,
            id,
            modelData,
            std::move(materials)))
    {
        std::fprintf(stderr, "Model을 만들지 못했습니다: %s\n", id.c_str());
        return nullptr;
    }

    materialCount_ += addedMaterialCount;
    Model* const modelPointer = model.get();
    models_[id] = std::move(model);
    return modelPointer;
}

Model* ResourceManager::LoadModel(const std::filesystem::path& filePath)
{
    // Scene File에 적을 수 있도록 구분자를 '/'로 통일한 경로를 모델의 이름으로 사용합니다.
    const std::string id = filePath.generic_u8string();
    if (Model* const cachedModel = FindModel(id))
    {
        return cachedModel;
    }

    ModelData modelData;
    if (!ModelLoader::LoadGltf(filePath, modelData))
    {
        return nullptr;
    }

    Model* const model = AddModel(id, std::move(modelData));
    if (model != nullptr)
    {
        std::printf("Model loaded: %s\n", id.c_str());
    }
    return model;
}

Model* ResourceManager::FindModel(const std::string& id)
{
    const auto cachedModel = models_.find(id);
    return cachedModel == models_.end() ? nullptr : cachedModel->second.get();
}

TextureHeap& ResourceManager::GetTextureHeap()
{
    return textureHeap_;
}

const TextureHeap& ResourceManager::GetTextureHeap() const
{
    return textureHeap_;
}

MaterialStyleLibrary& ResourceManager::GetStyles()
{
    return styles_;
}

std::size_t ResourceManager::GetMaterialCount() const
{
    return materialCount_;
}

bool ResourceManager::CreateWhiteTexture()
{
    ImageData whiteImage;
    whiteImage.width = 1;
    whiteImage.height = 1;
    whiteImage.pixels.assign(ImageData::BytesPerPixel, 255);

    UINT slot = 0;
    if (!whiteTexture_.CreateFromImage(
            device_,
            *commandContext_,
            whiteImage) ||
        !textureHeap_.Add(device_, whiteTexture_, slot))
    {
        return false;
    }

    whiteTexture_.SetDescriptorSlot(slot);
    return true;
}

bool ResourceManager::CreatePrimitiveModels()
{
    return AddModel("Cube", PrimitiveGeometry::CreateCube()) != nullptr &&
        AddModel("Sphere", PrimitiveGeometry::CreateSphere()) != nullptr &&
        AddModel("Plane", PrimitiveGeometry::CreatePlane()) != nullptr;
}

const Texture& ResourceManager::GetMaterialTexture(
    const ModelMaterialData& material)
{
    if (material.baseColorTexturePath.empty())
    {
        return whiteTexture_;
    }

    const Texture* const texture =
        LoadTexture(material.baseColorTexturePath.wstring());
    return texture != nullptr ? *texture : whiteTexture_;
}

void ResourceManager::Clear()
{
    models_.clear();
    textures_.clear();
}
