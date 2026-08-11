#pragma once

#include "D3D12Config.h"

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

#include <array>

// DirectX 12의 GPU 명령 기록, 제출, 완료 확인에 필요한 객체를 함께 관리합니다.
class D3D12CommandContext final
{
public:
    static constexpr UINT FrameCount = D3D12Config::FrameCount;

    D3D12CommandContext() = default;
    ~D3D12CommandContext();

    D3D12CommandContext(const D3D12CommandContext&) = delete;
    D3D12CommandContext& operator=(const D3D12CommandContext&) = delete;

    // Device로 Command Queue, Command 기록 객체, Fence 시스템을 순서대로 생성합니다.
    bool Initialize(ID3D12Device* device);

    // 해당 Frame의 GPU 작업 완료를 확인하고 Allocator와 CommandList를 다시 기록 가능한 상태로 만듭니다.
    bool BeginFrame(UINT frameIndex);

    // 기록을 끝낸 CommandList를 닫고 GPU Command Queue에 제출합니다.
    bool Execute();

    // 제출된 Frame의 완료 지점을 Fence 값으로 Command Queue 뒤에 기록합니다.
    bool SignalFrame(UINT frameIndex);

    // 현재 Command Queue에 먼저 제출된 모든 GPU 작업이 끝날 때까지 기다립니다.
    bool WaitForGPU();

    // SwapChain 생성에는 DirectX 12 Command Queue가 필요합니다.
    ID3D12CommandQueue* GetCommandQueue() const;

    // D3D12Context가 실제 Barrier와 렌더링 명령을 기록할 CommandList입니다.
    ID3D12GraphicsCommandList* GetCommandList() const;

private:
    bool CreateCommandQueue(ID3D12Device* device);
    bool CreateCommandObjects(ID3D12Device* device);
    bool CreateSynchronizationObjects(ID3D12Device* device);
    bool SignalFence(UINT64 fenceValue);
    bool WaitForFenceValue(UINT64 fenceValue);

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> commandAllocators_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    std::array<UINT64, FrameCount> frameFenceValues_{};
    UINT64 nextFenceValue_ = 1;
    HANDLE fenceEvent_ = nullptr;
};
