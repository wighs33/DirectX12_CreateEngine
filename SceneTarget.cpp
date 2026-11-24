#include "SceneTarget.h"

#include "D3D12Config.h"
#include "GpuBarrier.h"
#include "GpuTexture.h"

namespace
{
    // sRGB RTV에 저장되면 화면에서 (0.1, 0.2, 0.4)로 보이는 선형 색입니다.
    constexpr float ClearColor[] =
    {
        0.0100f,
        0.0331f,
        0.1329f,
        1.0f
    };
}

bool SceneTarget::Initialize(ID3D12Device* device, UINT width, UINT height)
{
    renderArea_ = RenderArea(width, height);
    return CreateColorBuffer(device, width, height) &&
        CreateRenderTargetView(device) &&
        depthBuffer_.Initialize(
            device,
            width,
            height,
            D3D12Config::SceneSampleCount,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void SceneTarget::Begin(ID3D12GraphicsCommandList* commandList) const
{
    renderArea_.Bind(commandList);

    // [9단계] Color 출력용 RTV와 깊이 비교/기록용 DSV를 Output Merger에 함께 연결합니다.
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap_.GetCpuHandle(0);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthBuffer_.GetDSV();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // [10단계] 물체를 그리기 전에 화면 전체를 지정한 색으로 지웁니다.
    commandList->ClearRenderTargetView(rtvHandle, ClearColor, 0, nullptr);

    depthBuffer_.Clear(commandList);
}

void SceneTarget::ResolveTo(
    ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* backBuffer) const
{
    GpuBarrier::Transition(
        commandList,
        colorBuffer_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

    // [17단계] 픽셀마다 표본의 평균을 BackBuffer에 씁니다.
    // 밝기가 맞도록 저장된 sRGB 값이 아니라 선형으로 되돌린 값의 평균을 냅니다.
    commandList->ResolveSubresource(
        backBuffer,
        0,
        colorBuffer_.Get(),
        0,
        D3D12Config::SceneTargetFormat);

    GpuBarrier::Transition(
        commandList,
        colorBuffer_.Get(),
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
}

const D3D12_VIEWPORT& SceneTarget::GetViewport() const
{
    return renderArea_.GetViewport();
}

bool SceneTarget::CreateColorBuffer(
    ID3D12Device* device,
    UINT width,
    UINT height)
{
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = D3D12Config::SceneTargetFormat;
    for (UINT channel = 0; channel < _countof(ClearColor); ++channel)
    {
        clearValue.Color[channel] = ClearColor[channel];
    }

    // SwapChain의 BackBuffer는 표본을 하나만 가질 수 있으므로 표본이 여러 개인 Texture를 따로 만듭니다.
    GpuTextureDescription description;
    description.width = width;
    description.height = height;
    description.format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    description.sampleCount = D3D12Config::SceneSampleCount;
    description.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    description.initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    description.clearValue = &clearValue;
    return GpuTexture::Create(device, description, colorBuffer_);
}

bool SceneTarget::CreateRenderTargetView(ID3D12Device* device)
{
    if (!descriptorHeap_.Initialize(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            1,
            false))
    {
        return false;
    }

    D3D12_RENDER_TARGET_VIEW_DESC description{};
    description.Format = D3D12Config::SceneTargetFormat;
    description.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
    device->CreateRenderTargetView(
        colorBuffer_.Get(),
        &description,
        descriptorHeap_.GetCpuHandle(0));
    return true;
}
