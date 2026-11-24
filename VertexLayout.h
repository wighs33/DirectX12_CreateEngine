#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

// 현재 Input Layout과 HLSL Vertex 입력에 대응하는 정점 데이터입니다.
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal;
    // 외곽선을 밀어낼 방향이며 OutlineNormalBaker가 채웁니다.
    DirectX::XMFLOAT3 outlineNormal{};
};

// Vertex의 어느 자리에 무엇이 있는지 Vertex Shader에 알려 줍니다.
class VertexLayout final
{
public:
    VertexLayout() = delete;

    static D3D12_INPUT_LAYOUT_DESC Get();
};
