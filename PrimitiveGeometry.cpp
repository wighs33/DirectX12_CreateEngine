#include "PrimitiveGeometry.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace
{
    ModelData CreateSinglePartModel(
        const std::string& name,
        std::vector<Vertex> vertices,
        std::vector<std::uint32_t> indices)
    {
        ModelPartData part;
        part.name = name;
        part.vertices = std::move(vertices);
        part.indices = std::move(indices);

        ModelData model;
        model.materials.push_back({ name, L"Assets/Textures/Test.png" });
        model.parts.push_back(std::move(part));
        return model;
    }
}

ModelData PrimitiveGeometry::CreateCube()
{
    // World/View/Projection 변환 전의 Object Space 원점에 한 변 길이가 1인 Cube를 정의합니다.
    // 같은 위치라도 면마다 UV가 다르므로 각 면에 네 Vertex를 두어 총 24개로 구성합니다.
    const DirectX::XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const DirectX::XMFLOAT3 frontNormal(0.0f, 0.0f, -1.0f);
    const DirectX::XMFLOAT3 backNormal(0.0f, 0.0f, 1.0f);
    const DirectX::XMFLOAT3 leftNormal(-1.0f, 0.0f, 0.0f);
    const DirectX::XMFLOAT3 rightNormal(1.0f, 0.0f, 0.0f);
    const DirectX::XMFLOAT3 topNormal(0.0f, 1.0f, 0.0f);
    const DirectX::XMFLOAT3 bottomNormal(0.0f, -1.0f, 0.0f);
    const Vertex cubeVertices[] =
    {
        // 앞면(-Z)
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), frontNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), frontNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), frontNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), frontNormal },

        // 뒷면(+Z)
        { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), backNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), backNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), backNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), backNormal },

        // 왼쪽 면(-X)
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), leftNormal },
        { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), leftNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), leftNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), leftNormal },

        // 오른쪽 면(+X)
        { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), rightNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), rightNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), rightNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), rightNormal },

        // 윗면(+Y)
        { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), topNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), topNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), topNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), topNormal },

        // 아랫면(-Y)
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), bottomNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), bottomNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), bottomNormal },
        { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), bottomNormal }
    };

    // 각 면의 바깥쪽이 Front Face가 되도록 Clockwise 화면 winding에 맞춰 12개 Triangle을 정의합니다.
    // 각 Index는 Vertex 자체가 아니라 Vertex Buffer 안의 몇 번째 Vertex를 사용할지 나타냅니다.
    const std::uint32_t cubeIndices[] =
    {
        // 앞면(-Z)
        0, 1, 2,
        0, 2, 3,

        // 뒷면(+Z)
        4, 5, 6,
        4, 6, 7,

        // 왼쪽 면(-X)
        8, 9, 10,
        8, 10, 11,

        // 오른쪽 면(+X)
        12, 13, 14,
        12, 14, 15,

        // 윗면(+Y)
        16, 17, 18,
        16, 18, 19,

        // 아랫면(-Y)
        20, 21, 22,
        20, 22, 23
    };

    return CreateSinglePartModel(
        "Cube",
        std::vector<Vertex>(std::begin(cubeVertices), std::end(cubeVertices)),
        std::vector<std::uint32_t>(std::begin(cubeIndices), std::end(cubeIndices)));
}

ModelData PrimitiveGeometry::CreateSphere()
{
    constexpr float radius = 0.5f;
    constexpr std::uint32_t stackCount = 16;
    constexpr std::uint32_t sliceCount = 32;
    const DirectX::XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);

    const std::size_t ringCount = stackCount - 1;
    const std::size_t verticesPerRing = sliceCount + 1;
    const std::size_t expectedVertexCount =
        2 + ringCount * verticesPerRing;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    vertices.reserve(expectedVertexCount);
    indices.reserve(
        static_cast<std::size_t>(sliceCount) * 3 * 2 +
        (ringCount - 1) * sliceCount * 6);

    // Pole은 하나의 Vertex로 두고 중간 Ring에만 UV Seam 중복 Vertex를 생성합니다.
    vertices.push_back(
        {
            DirectX::XMFLOAT3(0.0f, radius, 0.0f),
            white,
            DirectX::XMFLOAT2(0.5f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)
        });

    for (std::uint32_t stackIndex = 1;
         stackIndex < stackCount;
         ++stackIndex)
    {
        const float v =
            static_cast<float>(stackIndex) / static_cast<float>(stackCount);
        const float latitude = DirectX::XM_PI * v;
        const float sinLatitude = std::sin(latitude);
        const float cosLatitude = std::cos(latitude);

        for (std::uint32_t sliceIndex = 0;
             sliceIndex <= sliceCount;
             ++sliceIndex)
        {
            const float u =
                static_cast<float>(sliceIndex) /
                static_cast<float>(sliceCount);
            // 마지막 Slice는 첫 Slice와 Position을 정확히 공유하되 U만 1로 유지합니다.
            const float longitude = sliceIndex == sliceCount
                ? 0.0f
                : DirectX::XM_2PI * u;
            const float sinLongitude = std::sin(longitude);
            const float cosLongitude = std::cos(longitude);

            const DirectX::XMFLOAT3 normal(
                sinLatitude * sinLongitude,
                cosLatitude,
                sinLatitude * cosLongitude);
            const DirectX::XMFLOAT3 position(
                normal.x * radius,
                normal.y * radius,
                normal.z * radius);

            vertices.push_back(
                {
                    position,
                    white,
                    DirectX::XMFLOAT2(u, v),
                    normal
                });
        }
    }

    const std::uint32_t bottomPoleIndex =
        static_cast<std::uint32_t>(vertices.size());
    vertices.push_back(
        {
            DirectX::XMFLOAT3(0.0f, -radius, 0.0f),
            white,
            DirectX::XMFLOAT2(0.5f, 1.0f),
            DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f)
        });

    constexpr std::uint32_t topPoleIndex = 0;
    constexpr std::uint32_t firstRingStart = 1;
    for (std::uint32_t sliceIndex = 0;
         sliceIndex < sliceCount;
         ++sliceIndex)
    {
        indices.push_back(topPoleIndex);
        indices.push_back(static_cast<std::uint32_t>(
            firstRingStart + sliceIndex));
        indices.push_back(static_cast<std::uint32_t>(
            firstRingStart + sliceIndex + 1));
    }

    for (std::uint32_t ringIndex = 0;
         ringIndex + 1 < ringCount;
         ++ringIndex)
    {
        const std::uint32_t currentRingStart =
            static_cast<std::uint32_t>(
                firstRingStart + ringIndex * verticesPerRing);
        const std::uint32_t nextRingStart =
            static_cast<std::uint32_t>(
                currentRingStart + verticesPerRing);

        for (std::uint32_t sliceIndex = 0;
             sliceIndex < sliceCount;
             ++sliceIndex)
        {
            const std::uint32_t current = static_cast<std::uint32_t>(
                currentRingStart + sliceIndex);
            const std::uint32_t currentNext = static_cast<std::uint32_t>(
                current + 1);
            const std::uint32_t next = static_cast<std::uint32_t>(
                nextRingStart + sliceIndex);
            const std::uint32_t nextNext = static_cast<std::uint32_t>(
                next + 1);

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(currentNext);

            indices.push_back(currentNext);
            indices.push_back(next);
            indices.push_back(nextNext);
        }
    }

    const std::uint32_t lastRingStart = static_cast<std::uint32_t>(
        firstRingStart + (ringCount - 1) * verticesPerRing);
    for (std::uint32_t sliceIndex = 0;
         sliceIndex < sliceCount;
         ++sliceIndex)
    {
        indices.push_back(bottomPoleIndex);
        indices.push_back(static_cast<std::uint32_t>(
            lastRingStart + sliceIndex + 1));
        indices.push_back(static_cast<std::uint32_t>(
            lastRingStart + sliceIndex));
    }

    return CreateSinglePartModel(
        "Sphere",
        std::move(vertices),
        std::move(indices));
}
