#pragma once

#include <d3d12.h>
#include <wrl.h>

// Root Signature에 적은 칸의 순서입니다. 값을 연결할 때 같은 이름으로 칸을 찾습니다.
enum class RootParameter : UINT
{
    // b0 : 물체마다 바뀌는 행렬
    Object,
    // t0 : 재질의 Texture
    Texture,
    // b1 : 한 Frame 동안 모두가 쓰는 빛과 카메라
    Frame,
    // b2 : 재질마다 바뀌는 그림체 값
    Material,
    // t1 : 빛에서 본 깊이
    ShadowMap,
    Count
};

// Shader의 register와 C++가 넘기는 값을 잇는 목록을 만듭니다.
class D3D12RootSignature final
{
public:
    D3D12RootSignature() = default;
    ~D3D12RootSignature() = default;

    D3D12RootSignature(const D3D12RootSignature&) = delete;
    D3D12RootSignature& operator=(const D3D12RootSignature&) = delete;

    bool Initialize(ID3D12Device* device);

    ID3D12RootSignature* Get() const;

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
};
