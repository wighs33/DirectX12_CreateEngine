#include "D3D12Context.h"
#include "Input.h"

#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <utility>

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
    if (!CreateTexture())
        return false;
    if (!CreateCubeMesh())
        return false;

    // Scene이 소유할 세 GameObject는 각자 Transform을 가지며 하나뿐인 Cube Mesh를 함께 참조합니다.
    GameObject leftCube(&cubeMesh_);
    Transform& leftCubeTransform = leftCube.GetTransform();
    leftCubeTransform.SetPosition(-1.25f, 0.0f, 0.2f);
    leftCubeTransform.SetRotation(
        DirectX::XMConvertToRadians(-15.0f),
        DirectX::XMConvertToRadians(20.0f),
        0.0f);
    leftCubeTransform.SetScale(1.0f, 1.0f, 1.0f);

    GameObject centerCube(&cubeMesh_);
    Transform& centerCubeTransform = centerCube.GetTransform();
    centerCubeTransform.SetPosition(0.0f, 0.0f, 0.0f);
    centerCubeTransform.SetRotation(
        DirectX::XMConvertToRadians(-20.0f),
        DirectX::XMConvertToRadians(30.0f),
        0.0f);
    centerCubeTransform.SetScale(1.0f, 1.0f, 1.0f);

    GameObject rightCube(&cubeMesh_);
    Transform& rightCubeTransform = rightCube.GetTransform();
    rightCubeTransform.SetPosition(1.25f, 0.0f, -0.2f);
    rightCubeTransform.SetRotation(
        DirectX::XMConvertToRadians(-25.0f),
        DirectX::XMConvertToRadians(40.0f),
        0.0f);
    rightCubeTransform.SetScale(1.0f, 1.0f, 1.0f);

    // Scene은 GameObject 값을 소유하며 이후 Render는 Scene의 목록만 순회합니다.
    scene_.AddGameObject(std::move(leftCube));
    scene_.AddGameObject(std::move(centerCube));
    scene_.AddGameObject(std::move(rightCube));

    if (!CreateConstantBuffer(width, height))
        return false;
    if (!CreateLightingConstantBuffer())
        return false;

    return true;
}

void D3D12Context::Update(float deltaTime)
{
    constexpr float cameraSpeed = 3.0f;
    constexpr float mouseSensitivity = 0.002f;

    UpdateRenderingStatistics(deltaTime);

    Input::Update(window_);

    // Mouse Delta는 이미 한 Frame 동안 이동한 Pixel 양이므로 DeltaTime을 곱하지 않습니다.
    const MouseDelta mouseDelta = Input::GetMouseDelta();
    camera_.Rotate(
        static_cast<float>(mouseDelta.x) * mouseSensitivity,
        static_cast<float>(-mouseDelta.y) * mouseSensitivity);

    const float movementDistance = cameraSpeed * deltaTime;

    if (Input::IsKeyDown('W'))
        camera_.MoveForward(movementDistance);
    if (Input::IsKeyDown('S'))
        camera_.MoveForward(-movementDistance);
    if (Input::IsKeyDown('A'))
        camera_.MoveRight(-movementDistance);
    if (Input::IsKeyDown('D'))
        camera_.MoveRight(movementDistance);
}

bool D3D12Context::RenderFrame()
{
    renderingStats_.sceneObjectCount =
        static_cast<std::uint32_t>(scene_.GetGameObjectCount());
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
    const float aspectRatio =
        static_cast<float>(clientWidth_) / static_cast<float>(clientHeight_);
    const DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        aspectRatio,
        0.1f,
        100.0f);
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

    // Shader Visible SRV Heap과 그 첫 Descriptor를 Root Parameter 1의 t0 Table에 연결합니다.
    ID3D12DescriptorHeap* const descriptorHeaps[] =
    {
        srvDescriptorHeap_.Get()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    commandList->SetGraphicsRootDescriptorTable(
        1,
        srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(
        2,
        lightingConstantBuffer_->GetGPUVirtualAddress());

    // [8단계] Rasterizer가 Clip Space 결과를 변환할 화면 영역을 설정합니다.
    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    commandList->RSSetViewports(1, &viewport);

    // [9단계] Rasterizer가 실제 Pixel을 통과시킬 사각 영역을 설정합니다.
    const D3D12_RECT& scissorRect = swapChain_.GetScissorRect();
    commandList->RSSetScissorRects(1, &scissorRect);

    // [10단계] Color 출력용 RTV와 깊이 비교/기록용 DSV를 Output Merger에 함께 연결합니다.
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    constexpr float clearColor[] =
    {
        0.1f,
        0.2f,
        0.4f,
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
    const UINT64 objectCount = static_cast<UINT64>(scene_.GetGameObjectCount());
    for (std::size_t objectIndex = 0; objectIndex < gameObjects.size(); ++objectIndex)
    {
        const GameObject& gameObject = gameObjects[objectIndex];
        const Mesh* const mesh = gameObject.GetMesh();
        if (mesh == nullptr)
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

        const UINT64 constantBufferSlot =
            static_cast<UINT64>(frameIndex) * objectCount + objectIndex;
        const UINT64 constantBufferOffset =
            constantBufferSlot * AlignedConstantBufferSize;

        // [14단계] 같은 b0 Root CBV에 현재 Object 전용 Constant Buffer 주소를 연결합니다.
        commandList->SetGraphicsRootConstantBufferView(
            0,
            constantBuffer_->GetGPUVirtualAddress() + constantBufferOffset);

        // [15단계] 현재 GameObject가 참조하는 Mesh의 Vertex Buffer View를 Input Slot 0에 연결합니다.
        const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView =
            mesh->GetVertexBufferView();
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        // [16단계] 모든 Object가 공유하는 같은 Mesh의 Index Buffer View를 연결합니다.
        const D3D12_INDEX_BUFFER_VIEW& indexBufferView =
            mesh->GetIndexBufferView();
        commandList->IASetIndexBuffer(&indexBufferView);

        // [17단계] Object 하나당 한 번씩 Indexed Cube Draw 명령을 기록합니다.
        commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
        ++renderingStats_.renderedObjectCount;
        ++renderingStats_.drawCallCount;
        renderingStats_.triangleCount += mesh->GetIndexCount() / 3;
    }

    // [18단계] 렌더링을 마친 BackBuffer를 다시 화면 표시 가능한 상태로 전환합니다.
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

bool D3D12Context::CreateTexture()
{
    constexpr wchar_t textureFilePath[] = L"Assets/Textures/Test.png";
    ID3D12Device* const device = deviceResources_.GetDevice();

    Texture* const texture = resourceManager_.LoadTexture(textureFilePath);
    if (texture == nullptr)
    {
        return false;
    }

    // 같은 경로를 다시 요청해도 File Decode나 GPU Upload 없이 같은 Instance가 반환되는지 확인합니다.
    Texture* const cachedTexture = resourceManager_.LoadTexture(textureFilePath);
    if (cachedTexture != texture || resourceManager_.GetTexture(textureFilePath) != texture)
    {
        std::fprintf(stderr, "ResourceManager Texture Cache 검증에 실패했습니다.\n");
        return false;
    }

    // ResourceManager는 Texture를 소유하고 D3D12Context는 Shader Visible SRV Heap을 유지합니다.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDescription{};
    srvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDescription.NumDescriptors = 1;
    srvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvHeapDescription.NodeMask = 0;

    const HRESULT result = device->CreateDescriptorHeap(
        &srvHeapDescription,
        IID_PPV_ARGS(&srvDescriptorHeap_));
    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "SRV Descriptor Heap 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription{};
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescription.Format = texture->GetFormat();
    srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Texture2D.MostDetailedMip = 0;
    srvDescription.Texture2D.MipLevels = texture->GetMipLevels();
    srvDescription.Texture2D.PlaneSlice = 0;
    srvDescription.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(
        texture->GetResource(),
        &srvDescription,
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool D3D12Context::CreateCubeMesh()
{
    // World/View/Projection 변환 전의 Object Space 원점에 한 변 길이가 1인 Cube를 정의합니다.
    // 같은 위치라도 면마다 UV가 다르므로 각 면에 네 Vertex를 두어 총 24개로 구성합니다.
    const DirectX::XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const DirectX::XMFLOAT3 frontNormal(0.0f, 0.0f, -1.0f);
    const DirectX::XMFLOAT3 backNormal(0.0f, 0.0f, 1.0f);
    const DirectX::XMFLOAT3 leftNormal(-1.0f, 0.0f, 0.0f);
    const DirectX::XMFLOAT3 rightNormal(1.0f, 0.0f, 0.0f);
    const DirectX::XMFLOAT3 topNormal(0.0f, 1.0f, 0.0f);
    const DirectX::XMFLOAT3 bottomNormal(0.0f, -1.0f, 0.0f);
    const Vertex cubeVertices[] =
    {
        // 앞면(-Z)
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), frontNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), frontNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), frontNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), frontNormal },

        // 뒷면(+Z)
        { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), backNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), backNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), backNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), backNormal },

        // 왼쪽 면(-X)
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), leftNormal },
        { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), leftNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), leftNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), leftNormal },

        // 오른쪽 면(+X)
        { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), rightNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), rightNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), rightNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), rightNormal },

        // 윗면(+Y)
        { DirectX::XMFLOAT3(-0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), topNormal },
        { DirectX::XMFLOAT3(-0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), topNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), topNormal },
        { DirectX::XMFLOAT3( 0.5f,  0.5f, -0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), topNormal },

        // 아랫면(-Y)
        { DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 1.0f), bottomNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f, -0.5f), white, DirectX::XMFLOAT2(0.0f, 0.0f), bottomNormal },
        { DirectX::XMFLOAT3( 0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 0.0f), bottomNormal },
        { DirectX::XMFLOAT3(-0.5f, -0.5f,  0.5f), white, DirectX::XMFLOAT2(1.0f, 1.0f), bottomNormal }
    };

    // 각 면의 바깥쪽이 Front Face가 되도록 Clockwise 화면 winding에 맞춰 12개 Triangle을 정의합니다.
    // 각 Index는 Vertex 자체가 아니라 Vertex Buffer 안의 몇 번째 Vertex를 사용할지 나타냅니다.
    const std::uint16_t cubeIndices[] =
    {
        // 앞면(-Z)
        0, 1, 2,
        0, 2, 3,

        // 뒷면(+Z)
        4, 5, 6,
        4, 6, 7,

        // 왼쪽 면(-X)
        8, 9, 10,
        8, 10, 11,

        // 오른쪽 면(+X)
        12, 13, 14,
        12, 14, 15,

        // 윗면(+Y)
        16, 17, 18,
        16, 18, 19,

        // 아랫면(-Y)
        20, 21, 22,
        20, 22, 23
    };

    const UINT vertexCount = static_cast<UINT>(sizeof(cubeVertices) / sizeof(cubeVertices[0]));
    const UINT indexCount = static_cast<UINT>(sizeof(cubeIndices) / sizeof(cubeIndices[0]));

    // Cube 데이터는 외부에서 정의하고 Mesh는 전달받은 Geometry로 GPU Buffer를 생성합니다.
    return cubeMesh_.Initialize(
        deviceResources_.GetDevice(),
        cubeVertices,
        vertexCount,
        cubeIndices,
        indexCount);
}

bool D3D12Context::CreateConstantBuffer(UINT width, UINT height)
{
    if (width == 0 || height == 0)
    {
        std::fprintf(stderr, "Constant Buffer 생성에 사용할 화면 크기가 0입니다.\n");
        return false;
    }

    const std::size_t objectCount = scene_.GetGameObjectCount();
    if (objectCount == 0)
    {
        std::fprintf(stderr, "Constant Buffer를 생성할 Scene Object가 없습니다.\n");
        return false;
    }

    clientWidth_ = width;
    clientHeight_ = height;

    // CPU가 Matrix를 직접 쓰고 GPU가 읽을 수 있도록 Upload Heap을 사용합니다.
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment = 0;
    // Frame별 영역 안에 Object별 영역을 두어 Frame 간 경합과 Object 간 덮어쓰기를 함께 방지합니다.
    resourceDescription.Width =
        static_cast<UINT64>(AlignedConstantBufferSize) *
        D3D12Config::FrameCount *
        objectCount;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    const HRESULT resourceResult = deviceResources_.GetDevice()->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantBuffer_));

    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "상수 버퍼용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    // 한 번 얻은 CPU 주소를 계속 보관하는 Persistent Mapping 방식으로 사용합니다.
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = constantBuffer_->Map(
        0,
        &readRange,
        &constantBufferMappedData_);

    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "상수 버퍼의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memset(
        constantBufferMappedData_,
        0,
        static_cast<SIZE_T>(resourceDescription.Width));

    return true;
}

bool D3D12Context::CreateLightingConstantBuffer()
{
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment = 0;
    resourceDescription.Width = AlignedLightingConstantBufferSize;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    const HRESULT resourceResult = deviceResources_.GetDevice()->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&lightingConstantBuffer_));
    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "Lighting Constant Buffer 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    // Direction은 빛이 진행하는 방향이며, lightColor.w는 Diffuse 강도입니다.
    const LightingConstantBufferData lightingData
    {
        DirectX::XMFLOAT4(0.35f, -0.75f, 0.55f, 0.0f),
        DirectX::XMFLOAT4(1.0f, 0.95f, 0.85f, 0.85f),
        DirectX::XMFLOAT4(0.18f, 0.20f, 0.24f, 0.0f)
    };

    void* mappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = lightingConstantBuffer_->Map(0, &readRange, &mappedData);
    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "Lighting Constant Buffer Map에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memset(mappedData, 0, AlignedLightingConstantBufferSize);
    std::memcpy(mappedData, &lightingData, sizeof(lightingData));
    const D3D12_RANGE writtenRange{ 0, sizeof(lightingData) };
    lightingConstantBuffer_->Unmap(0, &writtenRange);

    return true;
}

void D3D12Context::UpdateObjectConstantBuffer(
    UINT frameIndex,
    std::size_t objectIndex,
    const DirectX::XMMATRIX& worldMatrix,
    const DirectX::XMMATRIX& viewProjectionMatrix)
{
    const std::size_t objectCount = scene_.GetGameObjectCount();
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

    const std::size_t constantBufferSlot =
        static_cast<std::size_t>(frameIndex) * objectCount + objectIndex;
    unsigned char* const objectData =
        static_cast<unsigned char*>(constantBufferMappedData_) +
        static_cast<SIZE_T>(constantBufferSlot) * AlignedConstantBufferSize;
    std::memset(objectData, 0, AlignedConstantBufferSize);
    std::memcpy(
        objectData,
        &constantBufferData,
        sizeof(constantBufferData));
}
