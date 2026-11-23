#pragma once

#include "MaterialStyle.h"

#include <DirectXMath.h>

#include <cstddef>
#include <string>

class Texture;

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
    DirectX::XMFLOAT3 outlineColor{};
    float outlineWidth = 0.0f;
};

// 모델의 재질 하나입니다. 같은 모델을 쓰는 모든 물체가 함께 사용합니다.
class Material final
{
public:
    // texture의 수명은 ResourceManager가 관리합니다.
    // constantSlot은 재질 상수 버퍼에서 이 재질이 쓰는 칸입니다.
    Material(
        const std::string& name,
        const Texture& texture,
        const MaterialStyle& style,
        std::size_t constantSlot);

    const std::string& GetName() const;
    const Texture& GetTexture() const;
    std::size_t GetConstantSlot() const;
    bool HasOutline() const;

    // 화면에서 고른 그림체 값을 Shader가 계산할 값으로 바꿉니다.
    MaterialConstants CreateConstants() const;

private:
    std::string name_;
    const Texture* texture_;
    MaterialStyle style_;
    std::size_t constantSlot_;
};
