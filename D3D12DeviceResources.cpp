#include "D3D12DeviceResources.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

bool D3D12DeviceResources::Initialize()
{
    // GPU와 Device 준비 흐름 : Debug Layer -> DXGI Factory -> GPU Adapter -> D3D12 Device
    if (!EnableDebugLayer())
        return false;
    if (!CreateFactory())
        return false;
    if (!FindAdapter())
        return false;
    if (!CreateDevice())
        return false;

    return true;
}

IDXGIFactory6* D3D12DeviceResources::GetFactory() const
{
    return factory_.Get();
}

ID3D12Device* D3D12DeviceResources::GetDevice() const
{
    return device_.Get();
}

bool D3D12DeviceResources::EnableDebugLayer()
{
#ifdef _DEBUG
    // [1단계] Device 생성 전에 DirectX API 사용을 검사할 Debug Layer를 활성화합니다.
    // DirectX API의 잘못된 사용 예 : 리소스 상태를 잘못 사용하는 경우 / 잘못된 매개변수를 전달한 경우 / CommandList 사용 순서가 잘못된 경우 / Descriptor를 잘못 연결한 경우 등등
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_))))
    {
        return false;
    }

    debugController_->EnableDebugLayer();
    factoryFlags_ |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    return true;
}

bool D3D12DeviceResources::CreateFactory()
{
    // [2단계] 시스템의 그래픽 Adapter를 탐색할 DXGI Factory를 생성합니다.
    // Factory : 객체를 찾거나 생성하는 역할을 맡은 관리자 객체
    return SUCCEEDED(CreateDXGIFactory2(factoryFlags_, IID_PPV_ARGS(&factory_)));
}

bool D3D12DeviceResources::FindAdapter()
{
    // [3단계] 고성능 GPU부터 확인해 D3D12 Device를 만들 수 있는 Adapter를 찾습니다.
    for (UINT adapterIndex = 0; ; ++adapterIndex)
    {
        const HRESULT result = factory_->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter_));

        if (result == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }
        if (FAILED(result))
        {
            return false;
        }

        DXGI_ADAPTER_DESC1 adapterDescription{};
        adapter_->GetDesc1(&adapterDescription);

        if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(
                adapter_.Get(),
                D3D_FEATURE_LEVEL_11_0,
                __uuidof(ID3D12Device),
                nullptr)))
        {
            return true;
        }

        adapter_.Reset();
    }

    return false;
}

bool D3D12DeviceResources::CreateDevice()
{
    // [4단계] 선택한 Adapter로 이후 모든 D3D12 객체의 출발점인 Device를 생성합니다.
    return SUCCEEDED(D3D12CreateDevice(
        adapter_.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device_)));
}
