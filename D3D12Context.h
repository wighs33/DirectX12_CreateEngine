#pragma once

#include "Camera.h"
#include "D3D12CommandContext.h"
#include "D3D12Config.h"
#include "D3D12DeviceResources.h"
#include "D3D12Pipeline.h"
#include "D3D12SwapChain.h"
#include "Frustum.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Scene.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

struct ImGui_ImplDX12_InitInfo;

struct RenderingStats
{
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    std::uint32_t sceneObjectCount = 0;
    std::uint32_t renderedObjectCount = 0;
    std::uint32_t culledObjectCount = 0;
    std::uint32_t drawCallCount = 0;
    std::uint32_t triangleCount = 0;
};

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

    // Keyboard/Mouse 입력과 DeltaTime을 이용해 CPU 측 Camera 상태를 갱신합니다.
    void Update(float deltaTime);

    // 현재 BackBuffer를 지정한 색으로 지우고 화면에 표시하는 한 프레임을 실행합니다.
    bool RenderFrame();

private:
    struct ConstantBufferData
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
        DirectX::XMFLOAT4X4 worldInverseTranspose;
    };

    struct LightingConstantBufferData
    {
        DirectX::XMFLOAT4 lightDirection;
        DirectX::XMFLOAT4 lightColor;
        DirectX::XMFLOAT4 ambientColor;
    };

    static constexpr UINT ConstantBufferAlignment =
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    static constexpr UINT AlignedConstantBufferSize =
        (static_cast<UINT>(sizeof(ConstantBufferData)) + ConstantBufferAlignment - 1) &
        ~(ConstantBufferAlignment - 1);
    static constexpr UINT AlignedLightingConstantBufferSize =
        (static_cast<UINT>(sizeof(LightingConstantBufferData)) + ConstantBufferAlignment - 1) &
        ~(ConstantBufferAlignment - 1);
    static constexpr UINT ImGuiDescriptorCount = 16;
    static constexpr std::size_t InvalidObjectIndex =
        (std::numeric_limits<std::size_t>::max)();

    bool CreateCubeMesh();
    bool CreateSphereMesh();
    bool CreateTexture();
    bool CreateConstantBuffer(UINT width, UINT height);
    bool EnsureConstantBufferCapacity(std::size_t requiredCapacity);
    bool RecreateConstantBuffer(std::size_t newCapacity);
    bool CreateLightingConstantBuffer();
    bool InitializeImGui();
    void ShutdownImGui();
    void BeginImGuiFrame();
    void DrawEditorUI();
    void ProcessObjectDeletionRequest();
    static void AllocateImGuiDescriptor(
        ImGui_ImplDX12_InitInfo* information,
        D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);
    static void FreeImGuiDescriptor(
        ImGui_ImplDX12_InitInfo* information,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);
    void UpdateObjectConstantBuffer(
        UINT frameIndex,
        std::size_t objectIndex,
        const DirectX::XMMATRIX& worldMatrix,
        const DirectX::XMMATRIX& viewProjectionMatrix);
    void UpdateRenderingStatistics(float deltaTime);
    void UpdateWindowTitle() const;

    // Camera는 Scene을 바라보는 View Matrix만 제공하며 GameObject와 별도로 유지합니다.
    Camera camera_;
    Frustum frustum_;
    // Camera 입력을 해석하는 Application 측 상태이며 RMB를 새로 누른 경우에만 활성화됩니다.
    bool isCameraLookActive_ = false;

    D3D12DeviceResources deviceResources_;
    D3D12SwapChain swapChain_;
    D3D12Pipeline pipeline_;

    // Mesh를 Scene보다 먼저 생성하고 나중에 파괴하여 GameObject의 비소유 Mesh 포인터를 안전하게 유지합니다.
    Mesh cubeMesh_;
    Mesh sphereMesh_;
    Scene scene_;

    // ResourceManager는 공유 Texture의 수명을 소유하고 SRV Heap은 Shader Binding을 담당합니다.
    ResourceManager resourceManager_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;

    // Scene SRV Heap과 분리된 Editor 전용 Shader Visible Heap입니다.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiDescriptorHeap_;
    std::vector<UINT> imguiFreeDescriptorIndices_;
    UINT imguiDescriptorSize_ = 0;
    bool imguiInitialized_ = false;
    // Selection은 Scene Object 자체가 아닌 Editor가 관리하며 vector 재배치에 안전한 Index를 사용합니다.
    std::size_t selectedObjectIndex_ = InvalidObjectIndex;
    std::uint32_t nextCubeNameIndex_ = 3;
    std::uint32_t nextSphereNameIndex_ = 0;
    bool deleteObjectRequested_ = false;
    bool deleteKeyWasDown_ = false;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> lightingConstantBuffer_;
    // Upload Heap을 한 번 Map한 뒤 Frame별 WVP Matrix를 갱신할 때 계속 사용할 CPU 주소입니다.
    void* constantBufferMappedData_ = nullptr;
    std::size_t objectConstantBufferCapacity_ = 0;
    RenderingStats renderingStats_;
    float statisticsElapsedTime_ = 0.0f;
    std::uint32_t statisticsFrameCount_ = 0;
    UINT clientWidth_ = 0;
    UINT clientHeight_ = 0;
    HWND window_ = nullptr;

    // 마지막에 선언하여 소멸 시 다른 GPU Resource보다 먼저 대기와 동기화 정리를 수행합니다.
    D3D12CommandContext commandContext_;
};
