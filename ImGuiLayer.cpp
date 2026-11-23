#include "ImGuiLayer.h"

#include "D3D12CommandContext.h"
#include "D3D12Config.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"

#include <cstdio>

// imgui_impl_win32.h는 Windows.h 의존성을 퍼뜨리지 않기 위해 이 선언을 의도적으로 노출하지 않습니다.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

bool ImGuiLayer::HandleMessage(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ImGui::GetCurrentContext() != nullptr &&
        ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam) != 0;
}

bool ImGuiLayer::Initialize(
    ID3D12Device* device,
    D3D12CommandContext& commandContext,
    HWND window)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescription.NumDescriptors = DescriptorCount;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDescription.NodeMask = 0;

    const HRESULT heapResult = device->CreateDescriptorHeap(
        &heapDescription,
        IID_PPV_ARGS(&descriptorHeap_));
    if (FAILED(heapResult))
    {
        std::fprintf(
            stderr,
            "ImGui SRV Descriptor Heap 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(heapResult));
        return false;
    }

    descriptorSize_ = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    freeDescriptorIndices_.reserve(DescriptorCount);
    for (UINT descriptorIndex = DescriptorCount;
         descriptorIndex > 0;
         --descriptorIndex)
    {
        freeDescriptorIndices_.push_back(descriptorIndex - 1);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // 고정 Panel의 단순 Focus가 WASD를 계속 점유하지 않도록 전역 Keyboard Navigation은 사용하지 않습니다.
    // 향후 InputText 같은 실제 Keyboard Widget은 활성 상태에서 WantCaptureKeyboard를 별도로 설정합니다.
    // 이번 단계의 Panel은 코드에서 고정 배치하므로 실행 폴더에 imgui.ini를 만들지 않습니다.
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(window))
    {
        std::fprintf(stderr, "ImGui Win32 Backend 초기화에 실패했습니다.\n");
        ImGui::DestroyContext();
        freeDescriptorIndices_.clear();
        descriptorHeap_.Reset();
        return false;
    }

    // v1.92.9 Backend의 InitInfo API와 Descriptor 할당 Callback을 사용합니다.
    ImGui_ImplDX12_InitInfo initializationInformation{};
    initializationInformation.Device = device;
    initializationInformation.CommandQueue = commandContext.GetCommandQueue();
    initializationInformation.NumFramesInFlight = D3D12Config::FrameCount;
    initializationInformation.RTVFormat = D3D12Config::BackBufferFormat;
    initializationInformation.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    initializationInformation.UserData = this;
    initializationInformation.SrvDescriptorHeap = descriptorHeap_.Get();
    initializationInformation.SrvDescriptorAllocFn = AllocateDescriptor;
    initializationInformation.SrvDescriptorFreeFn = FreeDescriptor;

    if (!ImGui_ImplDX12_Init(&initializationInformation))
    {
        std::fprintf(stderr, "ImGui DirectX 12 Backend 초기화에 실패했습니다.\n");
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        freeDescriptorIndices_.clear();
        descriptorHeap_.Reset();
        return false;
    }

    isInitialized_ = true;
    return true;
}

void ImGuiLayer::Shutdown(D3D12CommandContext& commandContext)
{
    if (!isInitialized_)
    {
        return;
    }

    // Backend와 Font Texture를 해제하기 전에 제출된 ImGui Draw가 모두 끝나도록 기다립니다.
    commandContext.WaitForGPU();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    isInitialized_ = false;
    freeDescriptorIndices_.clear();
    descriptorHeap_.Reset();
}

void ImGuiLayer::BeginFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiLayer::Render(ID3D12GraphicsCommandList* commandList)
{
    ImGui::Render();
    ID3D12DescriptorHeap* const descriptorHeaps[] =
    {
        descriptorHeap_.Get()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiLayer::AllocateDescriptor(
    ImGui_ImplDX12_InitInfo* information,
    D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
{
    ImGuiLayer* const context = information != nullptr
        ? static_cast<ImGuiLayer*>(information->UserData)
        : nullptr;
    IM_ASSERT(context != nullptr);
    IM_ASSERT(cpuHandle != nullptr && gpuHandle != nullptr);
    IM_ASSERT(
        context != nullptr &&
        !context->freeDescriptorIndices_.empty());
    if (context == nullptr || cpuHandle == nullptr || gpuHandle == nullptr ||
        context->freeDescriptorIndices_.empty())
    {
        return;
    }

    const UINT descriptorIndex = context->freeDescriptorIndices_.back();
    context->freeDescriptorIndices_.pop_back();

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuStart =
        context->descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE gpuStart =
        context->descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    cpuHandle->ptr = cpuStart.ptr +
        static_cast<SIZE_T>(descriptorIndex) * context->descriptorSize_;
    gpuHandle->ptr = gpuStart.ptr +
        static_cast<UINT64>(descriptorIndex) * context->descriptorSize_;
}

void ImGuiLayer::FreeDescriptor(
    ImGui_ImplDX12_InitInfo* information,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    ImGuiLayer* const context = information != nullptr
        ? static_cast<ImGuiLayer*>(information->UserData)
        : nullptr;
    IM_ASSERT(context != nullptr);
    if (context == nullptr || context->descriptorSize_ == 0)
    {
        return;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuStart =
        context->descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE gpuStart =
        context->descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    const UINT cpuIndex = static_cast<UINT>(
        (cpuHandle.ptr - cpuStart.ptr) / context->descriptorSize_);
    const UINT gpuIndex = static_cast<UINT>(
        (gpuHandle.ptr - gpuStart.ptr) / context->descriptorSize_);

    IM_ASSERT(cpuIndex == gpuIndex);
    IM_ASSERT(cpuIndex < DescriptorCount);
    if (cpuIndex == gpuIndex && cpuIndex < DescriptorCount)
    {
        context->freeDescriptorIndices_.push_back(cpuIndex);
    }
}
