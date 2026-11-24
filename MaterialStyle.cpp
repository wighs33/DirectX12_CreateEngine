#include "MaterialStyle.h"

namespace
{
    bool IsSameColor(
        const DirectX::XMFLOAT3& left,
        const DirectX::XMFLOAT3& right)
    {
        return left.x == right.x && left.y == right.y && left.z == right.z;
    }
}

bool operator==(const MaterialStyle& left, const MaterialStyle& right)
{
    return IsSameColor(left.shadeColor, right.shadeColor) &&
        left.shadeThreshold == right.shadeThreshold &&
        left.shadeSoftness == right.shadeSoftness &&
        left.ambientWeight == right.ambientWeight &&
        IsSameColor(left.rimColor, right.rimColor) &&
        left.rimStrength == right.rimStrength &&
        left.rimWidth == right.rimWidth &&
        IsSameColor(left.outlineColor, right.outlineColor) &&
        left.outlineWidth == right.outlineWidth;
}

bool operator!=(const MaterialStyle& left, const MaterialStyle& right)
{
    return !(left == right);
}
