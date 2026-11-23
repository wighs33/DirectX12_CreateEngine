#pragma once

#include "D3D12Pipeline.h"

#include <d3d12.h>

class D3D12RootSignature;
class ShadowMap;
struct RenderPassContext;

// 빛에서 본 장면의 깊이만 그림자 맵에 그립니다.
class ShadowPass final
{
public:
    ShadowPass() = default;
    ~ShadowPass() = default;

    ShadowPass(const ShadowPass&) = delete;
    ShadowPass& operator=(const ShadowPass&) = delete;

    bool Initialize(
        ID3D12Device* device,
        const D3D12RootSignature& rootSignature);

    // context에는 카메라가 아니라 빛의 시야에 들어온 물체를 담아 부릅니다.
    void Draw(
        const RenderPassContext& context,
        const ShadowMap& shadowMap) const;

private:
    D3D12Pipeline pipeline_;
};
