#include "D3D12CommandContext.h"

#include <cstdio>

D3D12CommandContext::~D3D12CommandContext()
{
    // 제출된 마지막 GPU 작업이 끝난 뒤 Command 객체를 해제합니다.
    if (commandQueue_ != nullptr && fence_.IsInitialized())
    {
        WaitForGPU();
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
    if (!fence_.Initialize(device))
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
    if (!fence_.Wait(frameFenceValues_[frameIndex]))
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
    return fence_.Signal(commandQueue_.Get(), frameFenceValues_[frameIndex]);
}

bool D3D12CommandContext::WaitForGPU()
{
    // Queue 끝에 Fence를 기록하고 그 값까지 기다리면 앞선 모든 GPU 작업이 완료됩니다.
    UINT64 fenceValue = 0;
    return fence_.Signal(commandQueue_.Get(), fenceValue) &&
        fence_.Wait(fenceValue);
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
