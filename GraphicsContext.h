#pragma once

#include <Windows.h>
#include <DirectXMath.h>

// DirectX 11과 DirectX 12 모두에서 사용할 수 있는 Vertex 데이터 형식입니다.
// 실제 GPU Buffer를 만드는 방법은 API마다 다르므로 각 Context가 담당합니다.
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

// 그래픽 API 종류와 관계없이 프로그램이 그래픽 Context에 요구하는 최소 기능입니다.
// 이후 D3D11Context를 추가한다면 같은 Initialize / RenderFrame 흐름을 구현할 수 있습니다.
class GraphicsContext
{
public:
    virtual ~GraphicsContext() = default;

    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    // Win32 창에 그래픽 API를 연결하고 필요한 객체를 초기화합니다.
    virtual bool Initialize(HWND window, UINT width, UINT height) = 0;

    // 선택한 그래픽 API로 한 프레임을 처리합니다.
    virtual bool RenderFrame() = 0;

protected:
    GraphicsContext() = default;
};
