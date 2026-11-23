#pragma once

#include "D3D12Pipeline.h"

#include <d3d12.h>

class D3D12RootSignature;
struct RenderPassContext;

// 보이는 물체의 뒷면을 부풀려 테두리를 그립니다.
class OutlinePass final
{
public:
    OutlinePass() = default;
    ~OutlinePass() = default;

    OutlinePass(const OutlinePass&) = delete;
    OutlinePass& operator=(const OutlinePass&) = delete;

    bool Initialize(
        ID3D12Device* device,
        const D3D12RootSignature& rootSignature);

    // 물체를 그린 뒤에 부르면 안쪽에 가려진 선은 깊이 비교에서 지워집니다.
    void Draw(const RenderPassContext& context) const;

private:
    D3D12Pipeline pipeline_;
};
