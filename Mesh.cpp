#include "Mesh.h"

#include <climits>
#include <cstdio>
#include <cstring>

bool Mesh::Initialize(
    ID3D12Device* device,
    const Vertex* vertices,
    UINT vertexCount,
    const std::uint16_t* indices,
    UINT indexCount)
{
    if (device == nullptr || vertices == nullptr || indices == nullptr ||
        vertexCount == 0 || indexCount == 0)
    {
        std::fprintf(stderr, "Mesh::Initialize에 유효하지 않은 Geometry 데이터가 전달되었습니다.\n");
        return false;
    }
    if (vertexCount > UINT_MAX / sizeof(Vertex) ||
        indexCount > UINT_MAX / sizeof(std::uint16_t))
    {
        std::fprintf(stderr, "Mesh Buffer 크기가 D3D12 Buffer View의 범위를 초과합니다.\n");
        return false;
    }

    vertexBuffer_.Reset();
    indexBuffer_.Reset();
    vertexBufferView_ = {};
    indexBufferView_ = {};
    vertexCount_ = 0;
    indexCount_ = 0;
    localBounds_ = {};

    if (!CreateVertexBuffer(device, vertices, vertexCount))
        return false;
    if (!CreateIndexBuffer(device, indices, indexCount))
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

    vertexCount_ = vertexCount;
    indexCount_ = indexCount;
    return true;
}

const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView() const
{
    return vertexBufferView_;
}

const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView() const
{
    return indexBufferView_;
}

UINT Mesh::GetVertexCount() const
{
    return vertexCount_;
}

UINT Mesh::GetIndexCount() const
{
    return indexCount_;
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
    const Vertex* vertices,
    UINT vertexCount)
{
    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertexCount);

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment = 0;
    resourceDescription.Width = vertexBufferSize;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    const HRESULT resourceResult = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer_));

    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "Mesh 정점 버퍼용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    void* mappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = vertexBuffer_->Map(0, &readRange, &mappedData);
    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "Mesh 정점 버퍼의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memcpy(mappedData, vertices, vertexBufferSize);
    const D3D12_RANGE writtenRange{ 0, static_cast<SIZE_T>(vertexBufferSize) };
    vertexBuffer_->Unmap(0, &writtenRange);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = vertexBufferSize;
    vertexBufferView_.StrideInBytes = static_cast<UINT>(sizeof(Vertex));
    return true;
}

bool Mesh::CreateIndexBuffer(
    ID3D12Device* device,
    const std::uint16_t* indices,
    UINT indexCount)
{
    const UINT indexBufferSize = static_cast<UINT>(sizeof(std::uint16_t) * indexCount);

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment = 0;
    resourceDescription.Width = indexBufferSize;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    const HRESULT resourceResult = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexBuffer_));

    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "Mesh 인덱스 버퍼용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    void* mappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = indexBuffer_->Map(0, &readRange, &mappedData);
    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "Mesh 인덱스 버퍼의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memcpy(mappedData, indices, indexBufferSize);
    const D3D12_RANGE writtenRange{ 0, static_cast<SIZE_T>(indexBufferSize) };
    indexBuffer_->Unmap(0, &writtenRange);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = indexBufferSize;
    indexBufferView_.Format = DXGI_FORMAT_R16_UINT;
    return true;
}
