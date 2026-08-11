#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

// DirectX 12를 사용할 GPU를 선택하고 모든 D3D12 객체의 출발점인 Device를 관리합니다.
class D3D12DeviceResources final
{
public:
    D3D12DeviceResources() = default;
    ~D3D12DeviceResources() = default;

    D3D12DeviceResources(const D3D12DeviceResources&) = delete;
    D3D12DeviceResources& operator=(const D3D12DeviceResources&) = delete;

    bool Initialize();

    IDXGIFactory6* GetFactory() const;
    ID3D12Device* GetDevice() const;

private:
    bool EnableDebugLayer();
    bool CreateFactory();
    bool FindAdapter();
    bool CreateDevice();

    UINT factoryFlags_ = 0;

#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController_;
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory_;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
};
