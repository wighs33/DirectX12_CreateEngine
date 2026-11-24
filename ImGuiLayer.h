#pragma once

#include "ImGuiDescriptorAllocator.h"

#include <d3d12.h>

class D3D12CommandContext;
struct ImGui_ImplDX12_InitInfo;

// ImGui Context와 Win32/DirectX 12 Backend의 수명을 관리합니다.
class ImGuiLayer final
{
public:
    ImGuiLayer() = default;
    ~ImGuiLayer() = default;

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    bool Initialize(
        ID3D12Device* device,
        D3D12CommandContext& commandContext,
        HWND window);

    // 창으로 들어온 메시지를 ImGui에 전달합니다. ImGui가 처리했으면 true를 반환합니다.
    static bool HandleMessage(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    // GPU가 제출된 ImGui Draw를 모두 끝낸 뒤 Backend와 Context를 해제합니다.
    void Shutdown(D3D12CommandContext& commandContext);

    void BeginFrame();

    // 현재 연결된 Render Target 위에 이번 Frame의 ImGui를 그립니다.
    void Render(ID3D12GraphicsCommandList* commandList);

private:
    // Backend가 칸이 필요하거나 다 썼을 때 부르는 함수입니다.
    static void AllocateDescriptor(
        ImGui_ImplDX12_InitInfo* information,
        D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);
    static void FreeDescriptor(
        ImGui_ImplDX12_InitInfo* information,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    ImGuiDescriptorAllocator descriptorAllocator_;
    bool isInitialized_ = false;
};
