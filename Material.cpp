#include "Material.h"

#include <cmath>

namespace
{
    // 화면에 보이는 sRGB 값을 빛 계산에 쓰는 선형 값으로 바꿉니다.
    float ToLinear(float value)
    {
        return value <= 0.04045f
            ? value / 12.92f
            : std::pow((value + 0.055f) / 1.055f, 2.4f);
    }

    DirectX::XMFLOAT3 ToLinear(const DirectX::XMFLOAT3& color)
    {
        return DirectX::XMFLOAT3(
            ToLinear(color.x),
            ToLinear(color.y),
            ToLinear(color.z));
    }
}

Material::Material(
    const std::string& name,
    const Texture& texture,
    const MaterialStyle& style,
    std::size_t constantSlot)
    : name_(name),
      texture_(&texture),
      style_(style),
      constantSlot_(constantSlot)
{
}

const std::string& Material::GetName() const
{
    return name_;
}

const Texture& Material::GetTexture() const
{
    return *texture_;
}

std::size_t Material::GetConstantSlot() const
{
    return constantSlot_;
}

bool Material::HasOutline() const
{
    return style_.outlineWidth > 0.0f;
}

MaterialConstants Material::CreateConstants() const
{
    MaterialConstants constants;
    constants.shadeColor = ToLinear(style_.shadeColor);
    constants.shadeThreshold = style_.shadeThreshold;
    constants.shadeSoftness = style_.shadeSoftness;
    constants.ambientWeight = style_.ambientWeight;
    constants.rimColor = ToLinear(style_.rimColor);
    constants.rimStrength = style_.rimStrength;
    constants.rimWidth = style_.rimWidth;
    constants.outlineColor = ToLinear(style_.outlineColor);
    constants.outlineWidth = style_.outlineWidth;
    return constants;
}
