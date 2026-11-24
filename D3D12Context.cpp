#include "D3D12Context.h"

#include "GpuBarrier.h"

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
    if (!sceneTarget_.Initialize(device, width, height))
        return false;
    if (!resourceManager_.Initialize(device, commandContext_))
        return false;
    if (!sceneRenderer_.Initialize(device, resourceManager_.GetTextureHeap()))
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
    sceneEditor_.Update(deltaTime, sceneTarget_.GetViewport());
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

    // [5~6단계] 그릴 물체를 고르고 모든 패스가 함께 쓰는 것을 연결합니다.
    sceneRenderer_.BeginFrame(
        commandList,
        frameIndex,
        scene_,
        camera_,
        resourceManager_,
        statistics_);

    // 화면을 그리기 전에 7~16단계를 그림자 맵에 먼저 한 번 실행합니다.
    sceneRenderer_.RenderShadow(
        commandList,
        frameIndex,
        resourceManager_,
        statistics_);

    // [7~11단계] 그릴 영역과 대상을 정하고 이전 Frame의 색과 깊이를 지웁니다.
    sceneTarget_.Begin(commandList);

    // [12~16단계] 장면의 물체와 외곽선을 그립니다.
    sceneRenderer_.RenderScene(
        commandList,
        frameIndex,
        resourceManager_,
        statistics_);

    // [17단계] 표본의 평균을 내어 BackBuffer로 옮깁니다.
    ResolveScene(commandList, backBuffer);

    // 장면이 담긴 BackBuffer 위에 Editor를 덧그립니다.
    RenderEditor(commandList, frameIndex);

    // [18단계] Scene과 Editor 렌더링을 마친 BackBuffer를 다시 화면 표시 가능한 상태로 전환합니다.
    GpuBarrier::Transition(
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

void D3D12Context::ResolveScene(
    ID3D12GraphicsCommandList* commandList,
    ID3D12Resource* backBuffer)
{
    GpuBarrier::Transition(
        commandList,
        backBuffer,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RESOLVE_DEST);

    sceneTarget_.ResolveTo(commandList, backBuffer);

    GpuBarrier::Transition(
        commandList,
        backBuffer,
        D3D12_RESOURCE_STATE_RESOLVE_DEST,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void D3D12Context::RenderEditor(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex)
{
    // ImGui의 색은 이미 sRGB 값이므로 변환하지 않는 RTV에 그립니다.
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain_.GetRTV(frameIndex);
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    imguiLayer_.Render(commandList);
}
