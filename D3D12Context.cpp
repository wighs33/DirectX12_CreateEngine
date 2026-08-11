#include "D3D12Context.h"

#include <cstdio>
#include <cstring>

bool D3D12Context::Initialize(HWND window, UINT width, UINT height)
{
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
    if (!CreateVertexBuffer())
        return false;

    return true;
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

    // [4단계] 현재 Index에 대응하는 BackBuffer Resource와 RTV를 선택합니다.
    ID3D12GraphicsCommandList* const commandList = commandContext_.GetCommandList();
    ID3D12Resource* const backBuffer = swapChain_.GetBackBuffer(frameIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain_.GetRTV(frameIndex);

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

    // [7단계] Shader Resource Binding 규칙인 빈 Root Signature를 연결합니다.
    commandList->SetGraphicsRootSignature(pipeline_.GetRootSignature());

    // [8단계] Rasterizer가 Clip Space 결과를 변환할 화면 영역을 설정합니다.
    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    commandList->RSSetViewports(1, &viewport);

    // [9단계] Rasterizer가 실제 Pixel을 통과시킬 사각 영역을 설정합니다.
    const D3D12_RECT& scissorRect = swapChain_.GetScissorRect();
    commandList->RSSetScissorRects(1, &scissorRect);

    // [10단계] 현재 BackBuffer의 RTV를 Render Target으로 연결합니다.
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    constexpr float clearColor[] =
    {
        0.1f,
        0.2f,
        0.4f,
        1.0f
    };

    // [11단계] Triangle을 그리기 전에 BackBuffer 전체를 지정한 색으로 지웁니다.
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // [12단계] Input Assembler가 Vertex를 세 개씩 묶는 Triangle List를 사용하도록 설정합니다.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // [13단계] Input Slot 0에 기존 Vertex Buffer View 하나를 연결합니다.
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // [14단계] Vertex 3개로 인스턴스 하나를 그리는 Non-Indexed Draw 명령을 기록합니다.
    commandList->DrawInstanced(VertexCount, 1, 0, 0);

    // [15단계] 렌더링을 마친 BackBuffer를 다시 화면 표시 가능한 상태로 전환합니다.
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    // [16~17단계] CommandList Close 후 완성된 명령을 GPU Queue에 제출합니다.
    if (!commandContext_.Execute())
        return false;

    // [18단계] 렌더링된 BackBuffer를 화면에 표시하고 다음 BackBuffer로 교체합니다.
    if (!swapChain_.Present())
        return false;

    // [19단계] 이 Frame의 GPU 작업 완료 지점을 Fence 값으로 Queue 뒤에 기록합니다.
    return commandContext_.SignalFrame(frameIndex);
}

bool D3D12Context::CreateVertexBuffer()
{
    // [13단계] World/View/Projection 변환 없이 바로 보이도록 Clip Space에 Triangle을 정의합니다.
    // Vertex들은 메모리에 연속으로 놓이며 각 Vertex의 Position 뒤에 Color가 이어집니다.
    // 기본 Rasterizer의 뒷면 제거 기준에 맞게 위 -> 오른쪽 아래 -> 왼쪽 아래 순서로 배치합니다.
    const Vertex triangleVertices[VertexCount] =
    {
        {
            DirectX::XMFLOAT3(0.0f, 0.5f, 0.0f),
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f),
            DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)
        },
        {
            DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f),
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
        }
    };

    const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * VertexCount);

    // Upload Heap은 CPU가 Map하여 데이터를 쓸 수 있으므로 학습용 Vertex Buffer를 단순하게 만들 수 있습니다.
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    // Resource를 2D Texture가 아니라 연속된 바이트를 담는 Buffer로 정의합니다.
    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Alignment = 0;
    resourceDescription.Width = vertexBufferSize;
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
        IID_PPV_ARGS(&vertexBuffer_));

    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "정점 버퍼용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    // Map으로 Upload Heap의 CPU 주소를 얻고 세 Vertex를 연속된 Buffer 메모리에 복사합니다.
    void* mappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = vertexBuffer_->Map(0, &readRange, &mappedData);
    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "정점 버퍼의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memcpy(mappedData, triangleVertices, vertexBufferSize);

    // 복사한 전체 범위를 GPU가 읽을 수 있도록 알리고 CPU 매핑을 끝냅니다.
    const D3D12_RANGE writtenRange{ 0, static_cast<SIZE_T>(vertexBufferSize) };
    vertexBuffer_->Unmap(0, &writtenRange);

    // View는 Resource 자체가 아니라 Input Assembler가 Buffer를 읽는 위치와 간격을 설명합니다.
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = vertexBufferSize;
    vertexBufferView_.StrideInBytes = static_cast<UINT>(sizeof(Vertex));

    return true;
}
