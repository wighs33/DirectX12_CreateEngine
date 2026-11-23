#pragma once

#include "BoundingBox.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <cstdint>

// 현재 Input Layout과 HLSL Vertex 입력에 대응하는 Position + Color + UV 정점 데이터입니다.
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal;
};

// CPU Geometry로 DirectX 12 Vertex/Index Buffer를 만들고 렌더링에 필요한 View를 제공합니다.
class Mesh final
{
public:
    Mesh() = default;
    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    bool Initialize(
        ID3D12Device* device,
        const Vertex* vertices,
        UINT vertexCount,
        const std::uint32_t* indices,
        UINT indexCount);

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;
    UINT GetVertexCount() const;
    UINT GetIndexCount() const;
    const AABB& GetLocalBounds() const;

private:
    bool CreateVertexBuffer(
        ID3D12Device* device,
        const Vertex* vertices,
        UINT vertexCount);
    bool CreateIndexBuffer(
        ID3D12Device* device,
        const std::uint32_t* indices,
        UINT indexCount);
    bool CalculateLocalBounds(const Vertex* vertices, UINT vertexCount);

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    UINT vertexCount_ = 0;
    UINT indexCount_ = 0;
    AABB localBounds_{};
};
