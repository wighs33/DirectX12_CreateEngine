#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

// GPU가 어디까지 끝냈는지 번호로 표시하고 CPU가 그 번호까지 기다리게 합니다.
class D3D12Fence final
{
public:
    D3D12Fence() = default;
    ~D3D12Fence();

    D3D12Fence(const D3D12Fence&) = delete;
    D3D12Fence& operator=(const D3D12Fence&) = delete;

    bool Initialize(ID3D12Device* device);
    bool IsInitialized() const;

    // Queue의 앞선 작업이 모두 끝나면 새 번호가 적히도록 예약하고 그 번호를 돌려줍니다.
    bool Signal(ID3D12CommandQueue* commandQueue, UINT64& value);

    // GPU가 그 번호를 적을 때까지 기다립니다. 이미 지났으면 바로 돌아옵니다.
    bool Wait(UINT64 value) const;

private:
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    UINT64 nextValue_ = 1;
    HANDLE event_ = nullptr;
};
