#include "D3D12Context.h"

D3D12Context::~D3D12Context()
{
    imguiLayer_.Shutdown(commandContext_);
}

bool D3D12Context::Initialize(HWND window, UINT width, UINT height)
{
    window_ = window;

    // DirectX 12 초기화 흐름을 주제별 객체에 순서대로 위임합니다.
    if (!deviceResources_.Initialize())
        return false;

    ID3D12Device* const device = deviceResources_.GetDevice();

    if (!commandContext_.Initialize(device))
        return false;
    if (!swapChain_.Initialize(
        deviceResources_.GetFactory(),
        device,
        commandContext_.GetCommandQueue(),
        window,
        width,
        height))
    {
        return false;
    }
    if (!pipeline_.Initialize(device))
        return false;
    if (!resourceManager_.Initialize(device, commandContext_))
        return false;
    if (!imguiLayer_.Initialize(device, commandContext_, window))
        return false;

    camera_.SetAspectRatio(
        static_cast<float>(width) / static_cast<float>(height));
    sceneEditor_.Initialize(window, scene_, camera_, resourceManager_);

    return true;
}

void D3D12Context::Update(float deltaTime)
{
    imguiLayer_.BeginFrame();
    if (statistics_.Update(deltaTime))
    {
        SetWindowTextW(window_, statistics_.CreateTitle().c_str());
    }
    sceneEditor_.Update(deltaTime, swapChain_.GetViewport());
}

bool D3D12Context::RenderFrame()
{
    if (!sceneRenderer_.Prepare(
            deviceResources_.GetDevice(),
            commandContext_,
            scene_,
            resourceManager_))
    {
        return false;
    }
    statistics_.BeginFrame(scene_.GetGameObjectCount());

    // SwapChain에서 이번 Frame Resource를 식별할 현재 BackBuffer Index를 먼저 얻습니다.
    const UINT frameIndex = swapChain_.GetCurrentFrameIndex();

    // [1~3단계] Fence 확인 -> CommandAllocator Reset -> CommandList Reset을 수행합니다.
    if (!commandContext_.BeginFrame(frameIndex))
    {
        return false;
    }

    // [4단계] 현재 Index에 대응하는 BackBuffer를 선택합니다.
    ID3D12GraphicsCommandList* const commandList = commandContext_.GetCommandList();
    ID3D12Resource* const backBuffer = swapChain_.GetBackBuffer(frameIndex);

    // [5단계] 화면 표시 상태의 BackBuffer를 렌더링 가능한 상태로 전환합니다.
    TransitionBackBuffer(
        commandList,
        backBuffer,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    // [6~10단계] 그릴 영역과 대상을 정하고 이전 Frame의 색과 깊이를 지웁니다.
    BindAndClearRenderTarget(commandList, frameIndex);

    // [11~17단계] 장면의 물체를 그립니다.
    sceneRenderer_.Render(
        commandList,
        frameIndex,
        pipeline_,
        scene_,
        camera_,
        resourceManager_,
        statistics_);

    // Scene 렌더링이 끝난 뒤 같은 BackBuffer 위에 Editor를 덧그립니다.
    RenderEditor(commandList, frameIndex);

    // [18단계] Scene과 Editor 렌더링을 마친 BackBuffer를 다시 화면 표시 가능한 상태로 전환합니다.
    TransitionBackBuffer(
        commandList,
        backBuffer,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);

    // [19~20단계] CommandList Close 후 완성된 명령을 GPU Queue에 제출합니다.
    if (!commandContext_.Execute())
        return false;

    // [21단계] 렌더링된 BackBuffer를 화면에 표시하고 다음 BackBuffer로 교체합니다.
    if (!swapChain_.Present())
        return false;

    // [22단계] 이 Frame의 GPU 작업 완료 지점을 Fence 값으로 Queue 뒤에 기록합니다.
    return commandContext_.SignalFrame(frameIndex);
}

void D3D12Context::TransitionBackBuffer(
    ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* backBuffer,
    D3D12_RESOURCE_STATES stateBefore,
    D3D12_RESOURCE_STATES stateAfter)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    commandList->ResourceBarrier(1, &barrier);
}

void D3D12Context::BindAndClearRenderTarget(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex)
{
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain_.GetRTV(frameIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = swapChain_.GetDSV();

    // [6단계] Rasterizer가 Clip Space 결과를 변환할 화면 영역을 설정합니다.
    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    commandList->RSSetViewports(1, &viewport);

    // [7단계] Rasterizer가 실제 Pixel을 통과시킬 사각 영역을 설정합니다.
    const D3D12_RECT& scissorRect = swapChain_.GetScissorRect();
    commandList->RSSetScissorRects(1, &scissorRect);

    // [8단계] Color 출력용 RTV와 깊이 비교/기록용 DSV를 Output Merger에 함께 연결합니다.
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // sRGB RTV에 저장되면 화면에서 (0.1, 0.2, 0.4)로 보이는 선형 색입니다.
    constexpr float clearColor[] =
    {
        0.0100f,
        0.0331f,
        0.1329f,
        1.0f
    };

    // [9단계] 물체를 그리기 전에 BackBuffer 전체를 지정한 색으로 지웁니다.
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // [10단계] 이전 Frame의 깊이가 남지 않도록 Depth Buffer 전체를 가장 먼 값 1.0으로 지웁니다.
    commandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);
}

void D3D12Context::RenderEditor(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex)
{
    // ImGui의 색은 이미 sRGB 값이므로 변환하지 않는 RTV로 바꿔 그립니다.
    const D3D12_CPU_DESCRIPTOR_HANDLE editorRtvHandle =
        swapChain_.GetEditorRTV(frameIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = swapChain_.GetDSV();
    commandList->OMSetRenderTargets(1, &editorRtvHandle, FALSE, &dsvHandle);
    imguiLayer_.Render(commandList);
}
