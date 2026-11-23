#pragma once

#include "D3D12Config.h"
#include "D3D12Fence.h"

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

#include <array>

// GPU에 시킬 명령을 기록하고 제출하며 Frame마다 끝났는지 확인합니다.
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

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> commandAllocators_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

    D3D12Fence fence_;
    // Frame마다 마지막으로 제출한 작업의 번호입니다.
    std::array<UINT64, FrameCount> frameFenceValues_{};
};
