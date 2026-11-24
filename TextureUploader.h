#pragma once

#include <d3d12.h>

class D3D12CommandContext;
struct ImageData;

// CPU에 풀어 둔 픽셀을 GPU 전용 Texture로 옮깁니다.
class TextureUploader final
{
public:
    TextureUploader() = delete;

    // texture는 COPY_DEST 상태여야 하며 다 옮기면 Pixel Shader가 읽는 상태가 됩니다.
    // GPU의 복사가 끝날 때까지 기다립니다.
    static bool Upload(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        const ImageData& image,
        ID3D12Resource* texture);
};
