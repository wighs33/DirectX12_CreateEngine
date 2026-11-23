#include "Mesh.h"

#include "GpuBuffer.h"

#include <climits>
#include <cstdio>

bool Mesh::Initialize(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const Vertex* vertices,
    UINT vertexCount,
    const std::uint32_t* indices,
    UINT indexCount)
{
    if (device == nullptr || vertices == nullptr || indices == nullptr ||
        vertexCount == 0 || indexCount == 0)
    {
        std::fprintf(stderr, "Mesh::Initialize에 유효하지 않은 Geometry 데이터가 전달되었습니다.\n");
        return false;
    }
    if (vertexCount > UINT_MAX / sizeof(Vertex) ||
        indexCount > UINT_MAX / sizeof(std::uint32_t))
    {
        std::fprintf(stderr, "Mesh Buffer 크기가 D3D12 Buffer View의 범위를 초과합니다.\n");
        return false;
    }

    vertexBuffer_.Reset();
    indexBuffer_.Reset();
    vertexBufferView_ = {};
    indexBufferView_ = {};
    indexCount_ = 0;
    localBounds_ = {};

    if (!CreateVertexBuffer(device, commandContext, vertices, vertexCount))
        return false;
    if (!CreateIndexBuffer(device, commandContext, indices, indexCount))
    {
        vertexBuffer_.Reset();
        vertexBufferView_ = {};
        return false;
    }
    if (!CalculateLocalBounds(vertices, vertexCount))
    {
        vertexBuffer_.Reset();
        indexBuffer_.Reset();
        vertexBufferView_ = {};
        indexBufferView_ = {};
        return false;
    }

    indexCount_ = indexCount;
    return true;
}

void Mesh::Draw(ID3D12GraphicsCommandList* commandList) const
{
    // [15단계] 현재 조각의 Vertex Buffer View를 Input Slot 0에 연결합니다.
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // [16단계] 같은 조각의 Index Buffer View를 연결합니다.
    commandList->IASetIndexBuffer(&indexBufferView_);

    // [17단계] 모델의 조각 하나당 한 번씩 Indexed Draw 명령을 기록합니다.
    commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

UINT Mesh::GetTriangleCount() const
{
    return indexCount_ / 3;
}

const AABB& Mesh::GetLocalBounds() const
{
    return localBounds_;
}

bool Mesh::CalculateLocalBounds(const Vertex* vertices, UINT vertexCount)
{
    if (vertices == nullptr || vertexCount == 0)
    {
        localBounds_ = {};
        return false;
    }

    DirectX::XMVECTOR minimum = DirectX::XMLoadFloat3(&vertices[0].position);
    DirectX::XMVECTOR maximum = minimum;

    for (UINT vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex)
    {
        const DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(&vertices[vertexIndex].position);
        minimum = DirectX::XMVectorMin(minimum, position);
        maximum = DirectX::XMVectorMax(maximum, position);
    }

    DirectX::XMStoreFloat3(&localBounds_.minimum, minimum);
    DirectX::XMStoreFloat3(&localBounds_.maximum, maximum);
    return true;
}

bool Mesh::CreateVertexBuffer(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const Vertex* vertices,
    UINT vertexCount)
{
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertexCount);
    if (!GpuBuffer::CreateDefault(
            device,
            commandContext,
            vertices,
            vertexBufferSize,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            vertexBuffer_))
    {
        return false;
    }

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = vertexBufferSize;
    vertexBufferView_.StrideInBytes = static_cast<UINT>(sizeof(Vertex));
    return true;
}

bool Mesh::CreateIndexBuffer(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    const std::uint32_t* indices,
    UINT indexCount)
{
    const UINT indexBufferSize = static_cast<UINT>(sizeof(std::uint32_t) * indexCount);
    if (!GpuBuffer::CreateDefault(
            device,
            commandContext,
            indices,
            indexBufferSize,
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            indexBuffer_))
    {
        return false;
    }

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = indexBufferSize;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    return true;
}
