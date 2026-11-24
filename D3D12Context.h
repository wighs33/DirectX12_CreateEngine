#pragma once

#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12DeviceResources.h"
#include "D3D12SwapChain.h"
#include "ImGuiLayer.h"
#include "RenderStatistics.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SceneEditor.h"
#include "SceneRenderer.h"
#include "SceneTarget.h"

#include <d3d12.h>

// 주제별 DirectX 12 객체를 연결하고 전체 초기화와 한 Frame의 렌더링 순서를 조정합니다.
class D3D12Context final
{
public:
    D3D12Context() = default;
    ~D3D12Context();

    D3D12Context(const D3D12Context&) = delete;
    D3D12Context& operator=(const D3D12Context&) = delete;

    // HWND에 연결할 DirectX 12 객체를 순서대로 생성합니다.
    bool Initialize(HWND window, UINT width, UINT height);

    // Editor Frame을 시작하고 입력과 편집 UI를 처리합니다.
    void Update(float deltaTime);

    // 장면과 편집 화면을 그리고 화면에 표시하는 한 프레임을 실행합니다.
    bool RenderFrame();

private:
    // 장면을 BackBuffer로 옮기고 그 위에 편집 화면을 그릴 수 있게 합니다.
    void ResolveScene(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* backBuffer);
    void RenderEditor(
        ID3D12GraphicsCommandList* commandList,
        UINT frameIndex);

    // Camera는 Scene을 바라보는 View Matrix만 제공하며 GameObject와 별도로 유지합니다.
    Camera camera_;

    D3D12DeviceResources deviceResources_;
    D3D12SwapChain swapChain_;
    SceneTarget sceneTarget_;

    // ResourceManager는 공유 Texture와 Model의 수명, 그리고 Texture를 연결할 SRV Heap을 소유합니다.
    // Scene보다 먼저 생성하고 나중에 파괴하여 GameObject의 비소유 Model 포인터를 안전하게 유지합니다.
    ResourceManager resourceManager_;
    Scene scene_;
    SceneRenderer sceneRenderer_;
    RenderStatistics statistics_;

    // Scene을 편집하는 UI와 그것을 그리는 ImGui Backend입니다.
    ImGuiLayer imguiLayer_;
    SceneEditor sceneEditor_;
    HWND window_ = nullptr;

    // 마지막에 선언하여 소멸 시 다른 GPU Resource보다 먼저 대기와 동기화 정리를 수행합니다.
    D3D12CommandContext commandContext_;
};
