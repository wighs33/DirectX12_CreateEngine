#include "D3D12Context.h"

#include "PrimitiveGeometry.h"
#include "Texture.h"

#include <cstdint>
#include <cstdio>
#include <cwchar>

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
    if (!resourceManager_.Initialize(device, commandContext_))
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
    if (!CreatePrimitiveModels())
        return false;

    if (!lightingConstants_.EnsureCapacity(device, commandContext_, 1))
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
    UpdateRenderingStatistics(deltaTime);
    sceneEditor_.Update(deltaTime, swapChain_.GetViewport());
}

bool D3D12Context::RenderFrame()
{
    const std::size_t sceneObjectCount = scene_.GetGameObjectCount();
    if (!objectConstants_.EnsureCapacity(
            deviceResources_.GetDevice(),
            commandContext_,
            sceneObjectCount) ||
        !materialConstants_.EnsureCapacity(
            deviceResources_.GetDevice(),
            commandContext_,
            resourceManager_.GetMaterialCount()))
    {
        return false;
    }

    renderingStats_.sceneObjectCount =
        static_cast<std::uint32_t>(sceneObjectCount);
    renderingStats_.renderedObjectCount = 0;
    renderingStats_.culledObjectCount = 0;
    renderingStats_.drawCallCount = 0;
    renderingStats_.triangleCount = 0;

    // SwapChain에서 이번 Frame Resource를 식별할 현재 BackBuffer Index를 먼저 얻습니다.
    const UINT frameIndex = swapChain_.GetCurrentFrameIndex();

    // [1~3단계] Fence 확인 -> CommandAllocator Reset -> CommandList Reset을 수행합니다.
    if (!commandContext_.BeginFrame(frameIndex))
    {
        return false;
    }

    const DirectX::XMMATRIX viewMatrix = camera_.GetViewMatrix();
    const DirectX::XMMATRIX projectionMatrix = camera_.GetProjectionMatrix();
    const DirectX::XMMATRIX viewProjectionMatrix = DirectX::XMMatrixMultiply(
        viewMatrix,
        projectionMatrix);

    // GPU 업로드용 Transpose Matrix가 아닌 CPU 원본 ViewProjection으로 Frame당 한 번 갱신합니다.
    frustum_.Update(viewProjectionMatrix);

    // [4단계] 현재 Index에 대응하는 BackBuffer/RTV와 공통 Depth Buffer의 DSV를 선택합니다.
    ID3D12GraphicsCommandList* const commandList = commandContext_.GetCommandList();
    ID3D12Resource* const backBuffer = swapChain_.GetBackBuffer(frameIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain_.GetRTV(frameIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = swapChain_.GetDSV();

    // [5단계] 화면 표시 상태의 BackBuffer를 렌더링 가능한 상태로 전환합니다.
    // ResourceBarrier : Resource의 사용 목적이 바뀐다는 것을 GPU에 명시적으로 알려주는 명령
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // [6단계] 이번 Draw에 사용할 Graphics Pipeline State를 연결합니다.
    commandList->SetPipelineState(pipeline_.GetPipelineState());

    // [7단계] b0 Constant Buffer와 t0 Texture를 사용할 Root Signature를 연결합니다.
    commandList->SetGraphicsRootSignature(pipeline_.GetRootSignature());

    // 모든 Texture의 SRV가 들어 있는 Heap을 연결하고 어느 칸을 쓸지는 조각마다 정합니다.
    const TextureHeap& textureHeap = resourceManager_.GetTextureHeap();
    ID3D12DescriptorHeap* const descriptorHeaps[] =
    {
        textureHeap.GetHeap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    UpdateLightingConstantBuffer(frameIndex);
    commandList->SetGraphicsRootConstantBufferView(
        2,
        lightingConstants_.GetGpuAddress(frameIndex, 0));

    // [8단계] Rasterizer가 Clip Space 결과를 변환할 화면 영역을 설정합니다.
    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    commandList->RSSetViewports(1, &viewport);

    // [9단계] Rasterizer가 실제 Pixel을 통과시킬 사각 영역을 설정합니다.
    const D3D12_RECT& scissorRect = swapChain_.GetScissorRect();
    commandList->RSSetScissorRects(1, &scissorRect);

    // [10단계] Color 출력용 RTV와 깊이 비교/기록용 DSV를 Output Merger에 함께 연결합니다.
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // sRGB RTV에 저장되면 화면에서 (0.1, 0.2, 0.4)로 보이는 선형 색입니다.
    constexpr float clearColor[] =
    {
        0.0100f,
        0.0331f,
        0.1329f,
        1.0f
    };

    // [11단계] Cube를 그리기 전에 BackBuffer 전체를 지정한 색으로 지웁니다.
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // [12단계] 이전 Frame의 깊이가 남지 않도록 Depth Buffer 전체를 가장 먼 값 1.0으로 지웁니다.
    commandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);

    // [13단계] Input Assembler가 Vertex를 세 개씩 묶는 Triangle List를 사용하도록 설정합니다.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Render는 Object를 직접 소유하지 않고 Scene이 제공하는 목록을 순회합니다.
    const std::vector<GameObject>& gameObjects = scene_.GetGameObjects();
    for (std::size_t objectIndex = 0; objectIndex < gameObjects.size(); ++objectIndex)
    {
        const GameObject& gameObject = gameObjects[objectIndex];
        const Model* const model = gameObject.GetModel();
        if (model == nullptr)
        {
            continue;
        }

        const AABB worldBounds = gameObject.GetWorldBounds();
        if (!frustum_.Intersects(worldBounds))
        {
            ++renderingStats_.culledObjectCount;
            continue;
        }

        const DirectX::XMMATRIX worldMatrix =
            gameObject.GetTransform().GetWorldMatrix();
        UpdateObjectConstantBuffer(
            frameIndex,
            objectIndex,
            worldMatrix,
            viewProjectionMatrix);

        // [14단계] 같은 b0 Root CBV에 현재 Object 전용 Constant Buffer 주소를 연결합니다.
        commandList->SetGraphicsRootConstantBufferView(
            0,
            objectConstants_.GetGpuAddress(frameIndex, objectIndex));

        ++renderingStats_.renderedObjectCount;
        for (const std::unique_ptr<ModelPart>& part : model->GetParts())
        {
            BindMaterial(
                commandList,
                frameIndex,
                model->GetMaterials()[part->materialIndex]);

            // [15단계] 현재 조각의 Vertex Buffer View를 Input Slot 0에 연결합니다.
            const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView =
                part->mesh.GetVertexBufferView();
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

            // [16단계] 같은 조각의 Index Buffer View를 연결합니다.
            const D3D12_INDEX_BUFFER_VIEW& indexBufferView =
                part->mesh.GetIndexBufferView();
            commandList->IASetIndexBuffer(&indexBufferView);

            // [17단계] 모델의 조각 하나당 한 번씩 Indexed Draw 명령을 기록합니다.
            commandList->DrawIndexedInstanced(
                part->mesh.GetIndexCount(),
                1,
                0,
                0,
                0);
            ++renderingStats_.drawCallCount;
            renderingStats_.triangleCount += part->mesh.GetIndexCount() / 3;
        }
    }

    // Scene 렌더링이 끝난 뒤 같은 BackBuffer 위에 Editor를 덧그립니다.
    // ImGui의 색은 이미 sRGB 값이므로 변환하지 않는 RTV로 바꿔 그립니다.
    const D3D12_CPU_DESCRIPTOR_HANDLE editorRtvHandle =
        swapChain_.GetEditorRTV(frameIndex);
    commandList->OMSetRenderTargets(1, &editorRtvHandle, FALSE, &dsvHandle);
    imguiLayer_.Render(commandList);

    // [18단계] Scene과 Editor 렌더링을 마친 BackBuffer를 다시 화면 표시 가능한 상태로 전환합니다.
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    // [19~20단계] CommandList Close 후 완성된 명령을 GPU Queue에 제출합니다.
    if (!commandContext_.Execute())
        return false;

    // [21단계] 렌더링된 BackBuffer를 화면에 표시하고 다음 BackBuffer로 교체합니다.
    if (!swapChain_.Present())
        return false;

    // [22단계] 이 Frame의 GPU 작업 완료 지점을 Fence 값으로 Queue 뒤에 기록합니다.
    return commandContext_.SignalFrame(frameIndex);
}

void D3D12Context::UpdateRenderingStatistics(float deltaTime)
{
    constexpr float statisticsUpdateInterval = 1.0f;

    statisticsElapsedTime_ += deltaTime;
    ++statisticsFrameCount_;

    if (statisticsElapsedTime_ < statisticsUpdateInterval)
    {
        return;
    }

    renderingStats_.fps =
        static_cast<float>(statisticsFrameCount_) / statisticsElapsedTime_;
    renderingStats_.frameTimeMs =
        statisticsElapsedTime_ * 1000.0f /
        static_cast<float>(statisticsFrameCount_);

    UpdateWindowTitle();

    statisticsElapsedTime_ = 0.0f;
    statisticsFrameCount_ = 0;
}

void D3D12Context::UpdateWindowTitle() const
{
    wchar_t title[256]{};
    swprintf_s(
        title,
        L"DirectX 12 Study | FPS: %.1f | Frame: %.2f ms | Objects: %u | Rendered: %u | Culled: %u | DrawCalls: %u | Triangles: %u",
        renderingStats_.fps,
        renderingStats_.frameTimeMs,
        renderingStats_.sceneObjectCount,
        renderingStats_.renderedObjectCount,
        renderingStats_.culledObjectCount,
        renderingStats_.drawCallCount,
        renderingStats_.triangleCount);
    SetWindowTextW(window_, title);
}

bool D3D12Context::CreatePrimitiveModels()
{
    return resourceManager_.AddModel("Cube", CreateCubeGeometry()) != nullptr &&
        resourceManager_.AddModel("Sphere", CreateSphereGeometry()) != nullptr;
}

void D3D12Context::UpdateLightingConstantBuffer(UINT frameIndex)
{
    // Direction은 빛이 진행하는 방향이며, lightColor.w는 Diffuse 강도입니다.
    const LightingConstantBufferData lightingData
    {
        DirectX::XMFLOAT4(0.35f, -0.75f, 0.55f, 0.0f),
        DirectX::XMFLOAT4(1.0f, 0.95f, 0.85f, 0.85f),
        DirectX::XMFLOAT4(0.18f, 0.20f, 0.24f, 0.0f)
    };
    lightingConstants_.Write(frameIndex, 0, &lightingData);
}

void D3D12Context::BindMaterial(
    ID3D12GraphicsCommandList* commandList,
    UINT frameIndex,
    const ModelMaterial& material)
{
    // Root Parameter 1의 t0 Table이 재질의 Texture 칸을 가리키게 합니다.
    commandList->SetGraphicsRootDescriptorTable(
        1,
        resourceManager_.GetTextureHeap().GetGpuHandle(
            material.texture->GetDescriptorSlot()));

    // Root Parameter 3의 b2에 재질의 그림체 값을 연결합니다.
    const MaterialConstants constants = CreateMaterialConstants(material.style);
    materialConstants_.Write(frameIndex, material.constantSlot, &constants);
    commandList->SetGraphicsRootConstantBufferView(
        3,
        materialConstants_.GetGpuAddress(frameIndex, material.constantSlot));
}

void D3D12Context::UpdateObjectConstantBuffer(
    UINT frameIndex,
    std::size_t objectIndex,
    const DirectX::XMMATRIX& worldMatrix,
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    const DirectX::XMMATRIX worldViewProjectionMatrix = DirectX::XMMatrixMultiply(
        worldMatrix,
        viewProjectionMatrix);
    const DirectX::XMMATRIX worldInverseTransposeMatrix = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixInverse(nullptr, worldMatrix));

    ConstantBufferData constantBufferData{};
    // DirectXMath의 행 우선 Matrix를 HLSL의 기본 열 우선 메모리 배치와 맞추기 위해 전치합니다.
    DirectX::XMStoreFloat4x4(
        &constantBufferData.worldViewProjection,
        DirectX::XMMatrixTranspose(worldViewProjectionMatrix));
    DirectX::XMStoreFloat4x4(
        &constantBufferData.worldInverseTranspose,
        DirectX::XMMatrixTranspose(worldInverseTransposeMatrix));

    objectConstants_.Write(frameIndex, objectIndex, &constantBufferData);
}
