#include "OutlineNormalBaker.h"

#include "ModelData.h"

#include <DirectXMath.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace
{
    // 파일로 내보낼 때 생긴 소수점 오차를 같은 자리로 묶는 격자의 크기입니다.
    constexpr float WeldGridSize = 0.0001f;
    // 서로 반대인 법선이 만나 합이 사라진 경우를 가려냅니다.
    constexpr float MinimumLengthSquared = 1.0e-6f;

    struct WeldKey
    {
        std::int64_t x = 0;
        std::int64_t y = 0;
        std::int64_t z = 0;

        bool operator==(const WeldKey& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct WeldKeyHash
    {
        std::size_t operator()(const WeldKey& key) const
        {
            const std::hash<std::int64_t> hash;
            return (hash(key.x) * 31 + hash(key.y)) * 31 + hash(key.z);
        }
    };

    WeldKey CreateWeldKey(const DirectX::XMFLOAT3& position)
    {
        return WeldKey
        {
            std::llround(position.x / WeldGridSize),
            std::llround(position.y / WeldGridSize),
            std::llround(position.z / WeldGridSize)
        };
    }
}

void OutlineNormalBaker::Bake(ModelData& modelData)
{
    // [1단계] 같은 자리의 법선을 모두 더합니다.
    std::unordered_map<WeldKey, DirectX::XMFLOAT3, WeldKeyHash> normalSums;
    for (const ModelPartData& part : modelData.parts)
    {
        for (const Vertex& vertex : part.vertices)
        {
            DirectX::XMFLOAT3& sum = normalSums[CreateWeldKey(vertex.position)];
            sum.x += vertex.normal.x;
            sum.y += vertex.normal.y;
            sum.z += vertex.normal.z;
        }
    }

    // [2단계] 더한 방향의 길이를 1로 맞춰 정점마다 적습니다.
    for (ModelPartData& part : modelData.parts)
    {
        for (Vertex& vertex : part.vertices)
        {
            const DirectX::XMVECTOR sum = DirectX::XMLoadFloat3(
                &normalSums[CreateWeldKey(vertex.position)]);
            if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(sum)) <
                MinimumLengthSquared)
            {
                vertex.outlineNormal = vertex.normal;
                continue;
            }

            DirectX::XMStoreFloat3(
                &vertex.outlineNormal,
                DirectX::XMVector3Normalize(sum));
        }
    }
}
