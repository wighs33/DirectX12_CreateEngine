#include "D3D12CommandContext.h"

#include <cstdio>

D3D12CommandContext::~D3D12CommandContext()
{
    // 제출된 마지막 GPU 작업이 끝난 뒤 Command 객체를 해제합니다.
    if (fenceEvent_ != nullptr && commandQueue_ != nullptr && fence_ != nullptr)
    {
        WaitForGPU();
    }

    // ComPtr 멤버는 자동 해제되지만 Win32 Event Handle은 직접 닫아야 합니다.
    if (fenceEvent_ != nullptr)
    {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

bool D3D12CommandContext::Initialize(ID3D12Device* device)
{
    if (device == nullptr)
    {
        std::fprintf(stderr, "D3D12CommandContext::Initialize에 유효하지 않은 디바이스 포인터가 전달되었습니다.\n");
        return false;
    }

    // GPU 제어 구조의 흐름 : Command Queue -> CommandAllocator/List -> Fence/Event
    if (!CreateCommandQueue(device))
        return false;
    if (!CreateCommandObjects(device))
        return false;
    if (!CreateSynchronizationObjects(device))
        return false;

    return true;
}

bool D3D12CommandContext::BeginFrame(UINT frameIndex)
{
    if (frameIndex >= FrameCount)
    {
        std::fprintf(stderr, "D3D12CommandContext::BeginFrame에 잘못된 프레임 인덱스가 전달되었습니다: %u\n", frameIndex);
        return false;
    }

    // 이 Frame의 Allocator를 GPU가 아직 사용 중이라면 완료될 때까지 기다립니다.
    if (!WaitForFenceValue(frameFenceValues_[frameIndex]))
    {
        return false;
    }

    // GPU 사용이 끝난 명령 메모리를 비우고 새로운 명령 기록에 재사용합니다.
    const HRESULT allocatorResetResult = commandAllocators_[frameIndex]->Reset();
    if (FAILED(allocatorResetResult))
    {
        std::fprintf(
            stderr,
            "ID3D12CommandAllocator::Reset 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(allocatorResetResult));
        return false;
    }

    // 해당 Allocator를 연결해 CommandList를 다시 Open 상태로 만듭니다.
    const HRESULT commandListResetResult = commandList_->Reset(
        commandAllocators_[frameIndex].Get(),
        nullptr);

    if (FAILED(commandListResetResult))
    {
        std::fprintf(
            stderr,
            "ID3D12GraphicsCommandList::Reset 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(commandListResetResult));
        return false;
    }

    return true;
}

bool D3D12CommandContext::Execute()
{
    // 명령 기록을 끝내고 완성된 CommandList를 GPU Queue에 제출합니다.
    const HRESULT closeResult = commandList_->Close();
    if (FAILED(closeResult))
    {
        std::fprintf(
            stderr,
            "ID3D12GraphicsCommandList::Close 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(closeResult));
        return false;
    }

    ID3D12CommandList* commandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, commandLists);
    return true;
}

bool D3D12CommandContext::SignalFrame(UINT frameIndex)
{
    if (frameIndex >= FrameCount)
    {
        std::fprintf(stderr, "D3D12CommandContext::SignalFrame에 잘못된 프레임 인덱스가 전달되었습니다: %u\n", frameIndex);
        return false;
    }

    // 이 Frame의 GPU 작업 완료 지점을 Fence 값으로 Queue 뒤에 기록합니다.
    const UINT64 submittedFenceValue = nextFenceValue_;
    if (!SignalFence(submittedFenceValue))
    {
        return false;
    }

    frameFenceValues_[frameIndex] = submittedFenceValue;
    ++nextFenceValue_;
    return true;
}

bool D3D12CommandContext::WaitForGPU()
{
    // Queue 끝에 Fence를 기록하고 그 값까지 기다리면 앞선 모든 GPU 작업이 완료됩니다.
    const UINT64 fenceValue = nextFenceValue_;
    if (!SignalFence(fenceValue))
    {
        return false;
    }

    ++nextFenceValue_;
    return WaitForFenceValue(fenceValue);
}

ID3D12CommandQueue* D3D12CommandContext::GetCommandQueue() const
{
    return commandQueue_.Get();
}

ID3D12GraphicsCommandList* D3D12CommandContext::GetCommandList() const
{
    return commandList_.Get();
}

bool D3D12CommandContext::CreateCommandQueue(ID3D12Device* device)
{
    // [5단계] GPU에 그래픽 명령을 제출할 DIRECT Command Queue를 생성합니다.
    D3D12_COMMAND_QUEUE_DESC description{};
    description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    description.NodeMask = 0;

    const HRESULT result = device->CreateCommandQueue(
        &description,
        IID_PPV_ARGS(&commandQueue_));

    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateCommandQueue 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    return true;
}

bool D3D12CommandContext::CreateCommandObjects(ID3D12Device* device)
{
    // [6단계] 각 BackBuffer Frame의 명령 메모리를 관리할 Allocator를 생성합니다.
    // CommandAllocator : CommandList에 기록된 명령을 보관하는 내부 메모리를 관리합니다.
    for (UINT frameIndex = 0; frameIndex < FrameCount; ++frameIndex)
    {
        const HRESULT result = device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocators_[frameIndex]));

        if (FAILED(result))
        {
            std::fprintf(
                stderr,
                "ID3D12Device::CreateCommandAllocator(%u) 호출에 실패했습니다. HRESULT: 0x%08X\n",
                frameIndex,
                static_cast<unsigned int>(result));
            return false;
        }
    }

    // 첫 번째 Frame의 Allocator로 하나의 재사용 가능한 Graphics CommandList를 생성합니다.
    // CommandList : GPU가 수행할 명령을 CPU에서 기록합니다.
    const HRESULT listResult = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocators_[0].Get(),
        nullptr,
        IID_PPV_ARGS(&commandList_));

    if (FAILED(listResult))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateCommandList 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(listResult));
        return false;
    }

    // CreateCommandList는 Open 상태로 생성되므로 명령이 없는 지금은 닫아 둡니다.
    const HRESULT closeResult = commandList_->Close();
    if (FAILED(closeResult))
    {
        std::fprintf(
            stderr,
            "ID3D12GraphicsCommandList::Close 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(closeResult));
        return false;
    }

    return true;
}

bool D3D12CommandContext::CreateSynchronizationObjects(ID3D12Device* device)
{
    // [7단계] GPU 완료 시점을 추적할 Fence를 초기값 0으로 생성합니다.
    // Fence : CPU는 특정 Resource나 CommandAllocator를 다시 사용하기 전에 GPU가 기존 작업을 끝냈는지 확인해야 합니다. 그 완료 시점을 표시하는 객체가 Fence입니다.
    constexpr UINT64 initialFenceValue = 0;
    const HRESULT fenceResult = device->CreateFence(
        initialFenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence_));

    if (FAILED(fenceResult))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateFence 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(fenceResult));
        return false;
    }

    frameFenceValues_.fill(initialFenceValue);
    nextFenceValue_ = initialFenceValue + 1;

    // GPU 완료 시 CPU를 깨울 Auto-reset Win32 Event를 생성합니다.
    fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent_ == nullptr)
    {
        std::fprintf(
            stderr,
            "CreateEventW 호출에 실패했습니다. Win32 오류 코드: 0x%08X\n",
            static_cast<unsigned int>(GetLastError()));
        return false;
    }

    return true;
}

bool D3D12CommandContext::SignalFence(UINT64 fenceValue)
{
    // Queue의 앞선 작업 뒤에 Fence 값을 기록하도록 예약합니다.
    const HRESULT result = commandQueue_->Signal(fence_.Get(), fenceValue);
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "ID3D12CommandQueue::Signal 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    return true;
}

bool D3D12CommandContext::WaitForFenceValue(UINT64 fenceValue)
{
    // 이미 완료한 값이면 CPU를 기다리게 하지 않습니다.
    if (fence_->GetCompletedValue() >= fenceValue)
    {
        return true;
    }

    const HRESULT eventResult = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
    if (FAILED(eventResult))
    {
        std::fprintf(
            stderr,
            "ID3D12Fence::SetEventOnCompletion 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(eventResult));
        return false;
    }

    const DWORD waitResult = WaitForSingleObject(fenceEvent_, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        std::fprintf(
            stderr,
            "WaitForSingleObject 호출에 실패했습니다. Win32 오류 코드: 0x%08X\n",
            static_cast<unsigned int>(GetLastError()));
        return false;
    }

    return true;
}
