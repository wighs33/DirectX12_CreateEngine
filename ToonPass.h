#pragma once

#include "D3D12Pipeline.h"

#include <d3d12.h>

class D3D12RootSignature;
struct RenderPassContext;

// 보이는 물체를 그림자를 받은 두 단 명암과 림라이트로 그립니다.
class ToonPass final
{
public:
    ToonPass() = default;
    ~ToonPass() = default;

    ToonPass(const ToonPass&) = delete;
    ToonPass& operator=(const ToonPass&) = delete;

    bool Initialize(
        ID3D12Device* device,
        const D3D12RootSignature& rootSignature);

    // 물체의 조각마다 재질의 값과 Texture를 연결해 그립니다.
    void Draw(const RenderPassContext& context) const;

private:
    D3D12Pipeline pipeline_;
};
