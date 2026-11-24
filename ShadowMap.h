#pragma once

#include "DepthBuffer.h"
#include "RenderArea.h"
#include "ShadowSettings.h"

#include <d3d12.h>

class TextureHeap;

// 빛에서 본 깊이를 담는 정사각형 Texture입니다. 한 Frame 안에서 깊이로 쓴 다음 Shader가 읽습니다.
class ShadowMap final
{
public:
    ShadowMap() = default;
    ~ShadowMap() = default;

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Shader가 읽을 SRV는 다른 Texture와 같은 Heap의 한 칸에 만듭니다.
    bool Initialize(ID3D12Device* device, TextureHeap& textureHeap);

    // 깊이를 쓰는 상태로 바꾸고 전체를 지운 뒤 그릴 대상으로 연결합니다.
    void BeginWrite(ID3D12GraphicsCommandList* commandList) const;

    // Pixel Shader가 읽는 상태로 되돌립니다.
    void EndWrite(ID3D12GraphicsCommandList* commandList) const;

    // Shader Visible Heap에서 이 그림자 맵의 SRV가 있는 칸입니다.
    UINT GetDescriptorSlot() const;

private:
    RenderArea renderArea_{ ShadowSettings::Resolution, ShadowSettings::Resolution };
    DepthBuffer depthBuffer_;
    UINT descriptorSlot_ = 0;
};
