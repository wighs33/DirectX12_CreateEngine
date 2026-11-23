#pragma once

#include <DirectXMath.h>

#include <filesystem>
#include <string>
#include <unordered_map>

// 재질 하나의 카툰 그림체 값입니다. 색은 화면에서 고르는 값과 같은 sRGB이고 Unity 재질의 값과 뜻이 같습니다.
struct MaterialStyle
{
    // 그늘에서 바탕색에 곱하는 색입니다. 검게 누르지 않고 재질마다 다른 색을 냅니다.
    DirectX::XMFLOAT3 shadeColor{ 0.68f, 0.64f, 0.84f };
    // 빛과 면이 이루는 값(-1~1)이 이보다 작으면 그늘입니다.
    float shadeThreshold = 0.0f;
    // 밝은 면과 그늘의 경계가 번지는 폭입니다.
    float shadeSoftness = 0.03f;
    // 주변광을 더하는 정도입니다.
    float ambientWeight = 0.25f;
    DirectX::XMFLOAT3 rimColor{ 1.0f, 1.0f, 1.0f };
    float rimStrength = 0.2f;
    float rimWidth = 0.3f;
};

// Shader의 b2와 같은 배치로 보낼 값입니다. 색은 빛 계산에 쓰는 선형 값으로 바뀌어 있습니다.
struct MaterialConstants
{
    DirectX::XMFLOAT3 shadeColor{};
    float shadeThreshold = 0.0f;
    DirectX::XMFLOAT3 rimColor{};
    float rimStrength = 0.0f;
    float shadeSoftness = 0.0f;
    float ambientWeight = 0.0f;
    float rimWidth = 0.0f;
    float padding = 0.0f;
};

MaterialConstants CreateMaterialConstants(const MaterialStyle& style);

// 재질 이름마다 정한 그림체 값을 JSON File에서 읽어 찾아 줍니다.
class MaterialStyleLibrary final
{
public:
    // 읽지 못하면 이유를 출력하고 모든 재질이 기본 그림체를 씁니다.
    bool Load(const std::filesystem::path& filePath);

    // 정해 둔 값이 없는 재질은 기본 그림체를 돌려줍니다.
    MaterialStyle Find(const std::string& materialName) const;

private:
    std::unordered_map<std::string, MaterialStyle> styles_;
};
