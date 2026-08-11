#include "D3D12Context.h"
#include "Input.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

bool D3D12Context::Initialize(HWND window, UINT width, UINT height)
{
    window_ = window;

    // 기존 Cube의 위치, 고정 회전과 크기를 Transform에 그대로 옮깁니다.
    cubeTransform_.SetPosition(0.0f, 0.0f, 0.0f);
    cubeTransform_.SetRotation(
        DirectX::XMConvertToRadians(-20.0f),
        DirectX::XMConvertToRadians(30.0f),
        0.0f);
    cubeTransform_.SetScale(1.0f, 1.0f, 1.0f);

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
    if (!CreateCubeMesh())
        return false;
    if (!CreateConstantBuffer(width, height))
        return false;

    return true;
}

void D3D12Context::Update(float deltaTime)
{
    constexpr float cameraSpeed = 3.0f;
    constexpr float mouseSensitivity = 0.002f;

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
    // SwapChain에서 이번 Frame Resource를 식별할 현재 BackBuffer Index를 먼저 얻습니다.
    const UINT frameIndex = swapChain_.GetCurrentFrameIndex();

    // [1~3단계] Fence 확인 -> CommandAllocator Reset -> CommandList Reset을 수행합니다.
    if (!commandContext_.BeginFrame(frameIndex))
    {
        return false;
    }

    // Fence 확인이 끝난 현재 Frame 슬롯에 이동된 Camera의 최신 WVP를 기록합니다.
    UpdateConstantBuffer(frameIndex);

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

    // [7단계] Vertex Shader가 b0 Constant Buffer를 사용할 Root Signature를 연결합니다.
    commandList->SetGraphicsRootSignature(pipeline_.GetRootSignature());

    // [8단계] Root Parameter 0에 WVP Constant Buffer의 GPU 주소를 연결합니다.
    commandList->SetGraphicsRootConstantBufferView(
        0,
        constantBuffer_->GetGPUVirtualAddress() +
        static_cast<UINT64>(frameIndex) * AlignedConstantBufferSize);

    // [9단계] Rasterizer가 Clip Space 결과를 변환할 화면 영역을 설정합니다.
    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    commandList->RSSetViewports(1, &viewport);

    // [10단계] Rasterizer가 실제 Pixel을 통과시킬 사각 영역을 설정합니다.
    const D3D12_RECT& scissorRect = swapChain_.GetScissorRect();
    commandList->RSSetScissorRects(1, &scissorRect);

    // [11단계] Color 출력용 RTV와 깊이 비교/기록용 DSV를 Output Merger에 함께 연결합니다.
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    constexpr float clearColor[] =
    {
        0.1f,
        0.2f,
        0.4f,
        1.0f
    };

    // [12단계] Cube를 그리기 전에 BackBuffer 전체를 지정한 색으로 지웁니다.
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // [13단계] 이전 Frame의 깊이가 남지 않도록 Depth Buffer 전체를 가장 먼 값 1.0으로 지웁니다.
    commandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);

    // [14단계] Input Assembler가 Vertex를 세 개씩 묶는 Triangle List를 사용하도록 설정합니다.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // [15단계] Mesh가 소유한 Vertex Buffer View를 Input Slot 0에 연결합니다.
    const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView =
        cubeMesh_.GetVertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // [16단계] Mesh가 소유한 Index Buffer View를 Input Assembler에 연결합니다.
    const D3D12_INDEX_BUFFER_VIEW& indexBufferView =
        cubeMesh_.GetIndexBufferView();
    commandList->IASetIndexBuffer(&indexBufferView);

    // [17단계] Mesh가 제공하는 Index 개수만큼 Cube를 그리는 명령을 기록합니다.
    commandList->DrawIndexedInstanced(cubeMesh_.GetIndexCount(), 1, 0, 0, 0);

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

bool D3D12Context::CreateCubeMesh()
{
    // World/View/Projection 변환 전의 Object Space 원점에 한 변 길이가 1인 Cube를 정의합니다.
    // Vertex들은 메모리에 연속으로 놓이며 각 Vertex의 Position 뒤에 Color가 이어집니다.
    const Vertex cubeVertices[] =
    {
        {
            DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f),
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f),
            DirectX::XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),
            DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),
            DirectX::XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)
        }
    };

    // 각 면의 바깥쪽이 Front Face가 되도록 Clockwise 화면 winding에 맞춰 12개 Triangle을 정의합니다.
    // 각 Index는 Vertex 자체가 아니라 Vertex Buffer 안의 몇 번째 Vertex를 사용할지 나타냅니다.
    const std::uint16_t cubeIndices[] =
    {
        // 앞면(-Z)
        0, 1, 2,
        0, 2, 3,

        // 뒷면(+Z)
        4, 7, 6,
        4, 6, 5,

        // 왼쪽 면(-X)
        0, 4, 5,
        0, 5, 1,

        // 오른쪽 면(+X)
        3, 2, 6,
        3, 6, 7,

        // 윗면(+Y)
        1, 5, 6,
        1, 6, 2,

        // 아랫면(-Y)
        0, 3, 7,
        0, 7, 4
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
    // GPU가 이전 Frame을 읽는 동안 CPU가 다음 Frame을 안전하게 쓸 수 있도록 Frame별 슬롯을 둡니다.
    resourceDescription.Width =
        static_cast<UINT64>(AlignedConstantBufferSize) * D3D12Config::FrameCount;
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

void D3D12Context::UpdateConstantBuffer(UINT frameIndex)
{
    // Transform이 Cube의 Position, Rotation, Scale을 조합한 World Matrix를 제공합니다.
    const DirectX::XMMATRIX worldMatrix = cubeTransform_.GetWorldMatrix();

    // Camera가 자신의 Position, Target과 Up을 사용해 View Matrix를 계산합니다.
    const DirectX::XMMATRIX viewMatrix = camera_.GetViewMatrix();

    const float aspectRatio =
        static_cast<float>(clientWidth_) / static_cast<float>(clientHeight_);
    const DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        aspectRatio,
        0.1f,
        100.0f);

    const DirectX::XMMATRIX worldViewMatrix = DirectX::XMMatrixMultiply(
        worldMatrix,
        viewMatrix);
    const DirectX::XMMATRIX worldViewProjectionMatrix = DirectX::XMMatrixMultiply(
        worldViewMatrix,
        projectionMatrix);

    ConstantBufferData constantBufferData{};
    // DirectXMath의 행 우선 Matrix를 HLSL의 기본 열 우선 메모리 배치와 맞추기 위해 전치합니다.
    DirectX::XMStoreFloat4x4(
        &constantBufferData.worldViewProjection,
        DirectX::XMMatrixTranspose(worldViewProjectionMatrix));

    unsigned char* const frameData =
        static_cast<unsigned char*>(constantBufferMappedData_) +
        static_cast<SIZE_T>(frameIndex) * AlignedConstantBufferSize;
    std::memset(frameData, 0, AlignedConstantBufferSize);
    std::memcpy(
        frameData,
        &constantBufferData,
        sizeof(constantBufferData));
}
