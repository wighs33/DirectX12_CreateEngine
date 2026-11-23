#include "D3D12Fence.h"

#include <cstdio>

D3D12Fence::~D3D12Fence()
{
    // ComPtr 멤버는 자동 해제되지만 Win32 Event Handle은 직접 닫아야 합니다.
    if (event_ != nullptr)
    {
        CloseHandle(event_);
    }
}

bool D3D12Fence::Initialize(ID3D12Device* device)
{
    // [7단계] GPU 완료 시점을 추적할 Fence를 초기값 0으로 생성합니다.
    // Fence : CPU는 특정 Resource나 CommandAllocator를 다시 사용하기 전에 GPU가 기존 작업을 끝냈는지 확인해야 합니다. 그 완료 시점을 표시하는 객체가 Fence입니다.
    constexpr UINT64 initialValue = 0;
    const HRESULT fenceResult = device->CreateFence(
        initialValue,
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

    nextValue_ = initialValue + 1;

    // GPU 완료 시 CPU를 깨울 Auto-reset Win32 Event를 생성합니다.
    event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (event_ == nullptr)
    {
        std::fprintf(
            stderr,
            "CreateEventW 호출에 실패했습니다. Win32 오류 코드: 0x%08X\n",
            static_cast<unsigned int>(GetLastError()));
        return false;
    }

    return true;
}

bool D3D12Fence::IsInitialized() const
{
    return fence_ != nullptr && event_ != nullptr;
}

bool D3D12Fence::Signal(ID3D12CommandQueue* commandQueue, UINT64& value)
{
    // Queue의 앞선 작업 뒤에 Fence 값을 기록하도록 예약합니다.
    const HRESULT result = commandQueue->Signal(fence_.Get(), nextValue_);
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "ID3D12CommandQueue::Signal 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    value = nextValue_;
    ++nextValue_;
    return true;
}

bool D3D12Fence::Wait(UINT64 value) const
{
    // 이미 완료한 값이면 CPU를 기다리게 하지 않습니다.
    if (fence_->GetCompletedValue() >= value)
    {
        return true;
    }

    const HRESULT eventResult = fence_->SetEventOnCompletion(value, event_);
    if (FAILED(eventResult))
    {
        std::fprintf(
            stderr,
            "ID3D12Fence::SetEventOnCompletion 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(eventResult));
        return false;
    }

    const DWORD waitResult = WaitForSingleObject(event_, INFINITE);
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
