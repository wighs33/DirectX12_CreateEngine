#include "D3D12Context.h"
#include "Input.h"
#include "PrimitiveGeometry.h"
#include "Ray.h"
#include "SceneSerializer.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

namespace
{
    const std::filesystem::path& GetSceneFilePath()
    {
        static const std::filesystem::path sceneFilePath =
            std::filesystem::path("Scenes") / "Scene.json";
        return sceneFilePath;
    }

    const std::filesystem::path& GetGuideFilePath()
    {
        static const std::filesystem::path guideFilePath =
            std::filesystem::path("Assets") / "Guide" / "EngineGuide.json";
        return guideFilePath;
    }

    const std::filesystem::path& GetModelDirectory()
    {
        static const std::filesystem::path modelDirectory =
            std::filesystem::path("Assets") / "Models";
        return modelDirectory;
    }

    std::vector<std::filesystem::path> FindModelFiles()
    {
        std::vector<std::filesystem::path> files;
        std::error_code directoryError;
        for (std::filesystem::recursive_directory_iterator entry(
                 GetModelDirectory(), directoryError), end;
             !directoryError && entry != end;
             entry.increment(directoryError))
        {
            if (entry->path().extension() == ".gltf")
            {
                files.push_back(entry->path());
            }
        }

        std::sort(files.begin(), files.end());
        return files;
    }
}

D3D12Context::~D3D12Context()
{
    ShutdownImGui();
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
    if (!CreateTexture())
        return false;
    if (!CreatePrimitiveModels())
        return false;

    if (!CreateConstantBuffer(width, height))
        return false;
    if (!CreateLightingConstantBuffer())
        return false;
    if (!InitializeImGui())
        return false;

    return true;
}

void D3D12Context::Update(float deltaTime)
{
    constexpr float cameraSpeed = 3.0f;
    constexpr float mouseSensitivity = 0.002f;

    BeginImGuiFrame();
    UpdateRenderingStatistics(deltaTime);

    Input::Update(window_);
    const ImGuiIO& io = ImGui::GetIO();

    // UI 밖에서 RMB를 새로 누른 경우에만 Editor Camera Look Mode를 시작합니다.
    if (Input::WasRightMouseButtonPressed())
    {
        isCameraLookActive_ =
            !io.WantCaptureMouse && !ImGuizmo::IsUsing();
        // 누르기 전 Cursor 이동이 첫 Camera Delta에 섞이지 않도록 현재 위치를 기준점으로 삼습니다.
        Input::ResetMousePosition(window_);
    }

    // RMB 해제, Focus 손실 또는 ImGui Mouse Capture 시 Look Mode를 즉시 끝냅니다.
    if (Input::WasRightMouseButtonReleased() ||
        (isCameraLookActive_ &&
            (io.WantCaptureMouse || !Input::IsRightMouseButtonDown())))
    {
        isCameraLookActive_ = false;
        Input::ResetMousePosition(window_);
    }

    // Delete 단축키는 눌린 첫 Frame에서만 요청하고 UI 입력 또는 Camera Look 중에는 무시합니다.
    const bool isDeleteKeyDown = Input::IsKeyDown(VK_DELETE);
    const bool wasDeleteKeyPressed = isDeleteKeyDown && !deleteKeyWasDown_;
    deleteKeyWasDown_ = isDeleteKeyDown;
    if (wasDeleteKeyPressed &&
        !io.WantCaptureKeyboard &&
        !isCameraLookActive_)
    {
        deleteObjectRequested_ = true;
    }

    // Hotkey와 UI 편집을 처리한 뒤 삭제로 Selection을 정리하고 마지막에 Gizmo를 갱신합니다.
    ProcessGizmoHotkeys();
    DrawEditorUI();
    ProcessObjectDeletionRequest();
    DrawTransformGizmo();

    const bool isGizmoUsing = ImGuizmo::IsUsing();

    // Mouse Delta는 이미 한 Frame 동안 이동한 Pixel 양이므로 DeltaTime을 곱하지 않습니다.
    if (isCameraLookActive_ && !isGizmoUsing)
    {
        const MouseDelta mouseDelta = Input::GetMouseDelta();
        camera_.Rotate(
            static_cast<float>(mouseDelta.x) * mouseSensitivity,
            static_cast<float>(-mouseDelta.y) * mouseSensitivity);
    }

    const float movementDistance = cameraSpeed * deltaTime;

    if (isCameraLookActive_ &&
        !isGizmoUsing &&
        !io.WantCaptureKeyboard)
    {
        if (Input::IsKeyDown('W'))
            camera_.MoveForward(movementDistance);
        if (Input::IsKeyDown('S'))
            camera_.MoveForward(-movementDistance);
        if (Input::IsKeyDown('A'))
            camera_.MoveRight(-movementDistance);
        if (Input::IsKeyDown('D'))
            camera_.MoveRight(movementDistance);
    }

    const std::vector<GameObject>& gameObjects = scene_.GetGameObjects();
    const bool hasValidSelection =
        selectedObjectIndex_ != InvalidObjectIndex &&
        selectedObjectIndex_ < gameObjects.size();
    const bool isGizmoInteraction =
        hasValidSelection &&
        (ImGuizmo::IsOver() || isGizmoUsing);

    POINT mousePosition{};
    if (Input::WasLeftMouseButtonPressed() &&
        !io.WantCaptureMouse &&
        !isGizmoInteraction &&
        !isCameraLookActive_ &&
        !Input::IsRightMouseButtonDown() &&
        Input::GetMousePosition(mousePosition))
    {
        PerformViewportPicking(mousePosition);
    }
}

bool D3D12Context::RenderFrame()
{
    const std::size_t sceneObjectCount = scene_.GetGameObjectCount();
    if (!EnsureConstantBufferCapacity(sceneObjectCount))
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
    const DirectX::XMMATRIX projectionMatrix = GetProjectionMatrix();
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

        const UINT64 constantBufferSlot =
            static_cast<UINT64>(frameIndex) * objectConstantBufferCapacity_ +
            objectIndex;
        const UINT64 constantBufferOffset =
            constantBufferSlot * AlignedConstantBufferSize;

        // [14단계] 같은 b0 Root CBV에 현재 Object 전용 Constant Buffer 주소를 연결합니다.
        commandList->SetGraphicsRootConstantBufferView(
            0,
            constantBuffer_->GetGPUVirtualAddress() + constantBufferOffset);

        ++renderingStats_.renderedObjectCount;
        for (const std::unique_ptr<ModelPart>& part : model->GetParts())
        {
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

    // Scene 렌더링이 끝난 뒤 ImGui 전용 Heap으로 전환하여 같은 BackBuffer 위에 Editor를 덧그립니다.
    ImGui::Render();
    ID3D12DescriptorHeap* const imguiDescriptorHeaps[] =
    {
        imguiDescriptorHeap_.Get()
    };
    commandList->SetDescriptorHeaps(
        _countof(imguiDescriptorHeaps),
        imguiDescriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

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

bool D3D12Context::InitializeImGui()
{
    ID3D12Device* const device = deviceResources_.GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
    heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescription.NumDescriptors = ImGuiDescriptorCount;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDescription.NodeMask = 0;

    const HRESULT heapResult = device->CreateDescriptorHeap(
        &heapDescription,
        IID_PPV_ARGS(&imguiDescriptorHeap_));
    if (FAILED(heapResult))
    {
        std::fprintf(
            stderr,
            "ImGui SRV Descriptor Heap 생성에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(heapResult));
        return false;
    }

    imguiDescriptorSize_ = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    imguiFreeDescriptorIndices_.reserve(ImGuiDescriptorCount);
    for (UINT descriptorIndex = ImGuiDescriptorCount;
         descriptorIndex > 0;
         --descriptorIndex)
    {
        imguiFreeDescriptorIndices_.push_back(descriptorIndex - 1);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // 고정 Panel의 단순 Focus가 WASD를 계속 점유하지 않도록 전역 Keyboard Navigation은 사용하지 않습니다.
    // 향후 InputText 같은 실제 Keyboard Widget은 활성 상태에서 WantCaptureKeyboard를 별도로 설정합니다.
    // 이번 단계의 Panel은 코드에서 고정 배치하므로 실행 폴더에 imgui.ini를 만들지 않습니다.
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    engineGuideWindow_.Initialize(GetGuideFilePath());

    if (!ImGui_ImplWin32_Init(window_))
    {
        std::fprintf(stderr, "ImGui Win32 Backend 초기화에 실패했습니다.\n");
        ImGui::DestroyContext();
        imguiFreeDescriptorIndices_.clear();
        imguiDescriptorHeap_.Reset();
        return false;
    }

    // v1.92.9 Backend의 InitInfo API와 Descriptor 할당 Callback을 사용합니다.
    ImGui_ImplDX12_InitInfo initializationInformation{};
    initializationInformation.Device = device;
    initializationInformation.CommandQueue = commandContext_.GetCommandQueue();
    initializationInformation.NumFramesInFlight = D3D12Config::FrameCount;
    initializationInformation.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initializationInformation.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    initializationInformation.UserData = this;
    initializationInformation.SrvDescriptorHeap = imguiDescriptorHeap_.Get();
    initializationInformation.SrvDescriptorAllocFn = AllocateImGuiDescriptor;
    initializationInformation.SrvDescriptorFreeFn = FreeImGuiDescriptor;

    if (!ImGui_ImplDX12_Init(&initializationInformation))
    {
        std::fprintf(stderr, "ImGui DirectX 12 Backend 초기화에 실패했습니다.\n");
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiFreeDescriptorIndices_.clear();
        imguiDescriptorHeap_.Reset();
        return false;
    }

    imguiInitialized_ = true;
    return true;
}

void D3D12Context::ShutdownImGui()
{
    if (!imguiInitialized_)
    {
        return;
    }

    // Backend와 Font Texture를 해제하기 전에 제출된 ImGui Draw가 모두 끝나도록 기다립니다.
    commandContext_.WaitForGPU();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    imguiInitialized_ = false;
    imguiFreeDescriptorIndices_.clear();
    imguiDescriptorHeap_.Reset();
}

void D3D12Context::BeginImGuiFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void D3D12Context::DrawEditorUI()
{
    constexpr float panelMargin = 10.0f;
    constexpr float panelGap = 10.0f;
    constexpr float leftPanelWidth = 220.0f;
    constexpr float rightPanelWidth = 280.0f;
    constexpr float createPanelHeight = 162.0f;
    constexpr float gizmoToolbarWidth = 320.0f;
    constexpr float gizmoToolbarHeight = 46.0f;
    constexpr float viewportHelpWidth = 327.0f;
    constexpr float viewportHelpHeight = 36.0f;
    constexpr ImGuiWindowFlags panelFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    const ImVec2 workPosition = viewport->WorkPos;
    const ImVec2 workSize = viewport->WorkSize;
    const float fullPanelHeight = (std::max)(
        100.0f,
        workSize.y - panelMargin * 2.0f);
    const float hierarchyPanelHeight = (std::max)(
        100.0f,
        workSize.y - createPanelHeight - panelMargin * 3.0f);
    const float inspectorX = (std::max)(
        workPosition.x + panelMargin,
        workPosition.x + workSize.x - rightPanelWidth - panelMargin);

    std::vector<GameObject>& gameObjects = scene_.GetGameObjects();
    if (selectedObjectIndex_ != InvalidObjectIndex &&
        selectedObjectIndex_ >= gameObjects.size())
    {
        selectedObjectIndex_ = InvalidObjectIndex;
    }

    ImGui::SetNextWindowPos(
        ImVec2(
            workPosition.x + (workSize.x - gizmoToolbarWidth) * 0.5f,
            workPosition.y + panelMargin),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(gizmoToolbarWidth, gizmoToolbarHeight),
        ImGuiCond_Always);
    ImGui::Begin(
        "Gizmo Toolbar",
        nullptr,
        panelFlags | ImGuiWindowFlags_NoTitleBar);
    if (ImGui::RadioButton(
            "Move (W)",
            gizmoOperation_ == GizmoOperation::Translate))
    {
        gizmoOperation_ = GizmoOperation::Translate;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(
            "Rotate (E)",
            gizmoOperation_ == GizmoOperation::Rotate))
    {
        gizmoOperation_ = GizmoOperation::Rotate;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(
            "Scale (R)",
            gizmoOperation_ == GizmoOperation::Scale))
    {
        gizmoOperation_ = GizmoOperation::Scale;
    }
    engineGuideWindow_.DrawHelpButton("feature-gizmo");
    ImGui::End();

    ImGui::SetNextWindowPos(
        ImVec2(workPosition.x + panelMargin, workPosition.y + panelMargin),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(leftPanelWidth, createPanelHeight),
        ImGuiCond_Always);
    ImGui::Begin("Create", nullptr, panelFlags);
    if (ImGui::Button("Cube", ImVec2(120.0f, 0.0f)))
    {
        AddSceneObject(resourceManager_.FindModel("Cube"), "Cube");
    }
    engineGuideWindow_.DrawHelpButtonAtRight("feature-create");
    if (ImGui::Button("Sphere", ImVec2(120.0f, 0.0f)))
    {
        AddSceneObject(resourceManager_.FindModel("Sphere"), "Sphere");
    }
    DrawModelPicker();

    ImGui::Separator();
    if (ImGui::Button("Save Scene", ImVec2(84.0f, 0.0f)))
    {
        SceneSerializer::SaveScene(scene_, GetSceneFilePath());
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene", ImVec2(84.0f, 0.0f)))
    {
        if (SceneSerializer::LoadScene(
                scene_,
                resourceManager_,
                GetSceneFilePath()))
        {
            selectedObjectIndex_ = InvalidObjectIndex;
            deleteObjectRequested_ = false;
        }
    }
    engineGuideWindow_.DrawHelpButtonAtRight("feature-save");
    ImGui::End();

    ImGui::SetNextWindowPos(
        ImVec2(
            workPosition.x + panelMargin,
            workPosition.y + panelMargin + createPanelHeight + panelGap),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(leftPanelWidth, hierarchyPanelHeight),
        ImGuiCond_Always);
    ImGui::Begin("Hierarchy", nullptr, panelFlags);
    ImGui::TextUnformatted("Objects");
    engineGuideWindow_.DrawHelpButtonAtRight("feature-hierarchy");
    ImGui::Separator();
    if (gameObjects.empty())
    {
        ImGui::TextDisabled("No Objects");
    }
    else
    {
        for (std::size_t objectIndex = 0;
             objectIndex < gameObjects.size();
             ++objectIndex)
        {
            const GameObject& gameObject = gameObjects[objectIndex];
            const bool isSelected = selectedObjectIndex_ == objectIndex;

            ImGui::PushID(static_cast<int>(objectIndex));
            if (ImGui::Selectable(gameObject.GetName().c_str(), isSelected))
            {
                selectedObjectIndex_ = objectIndex;
            }
            ImGui::PopID();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowPos(
        ImVec2(inspectorX, workPosition.y + panelMargin),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(rightPanelWidth, fullPanelHeight),
        ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr, panelFlags);
    if (selectedObjectIndex_ == InvalidObjectIndex)
    {
        ImGui::TextDisabled("No Object Selected");
    }
    else
    {
        GameObject& selectedObject = gameObjects[selectedObjectIndex_];
        Transform& transform = selectedObject.GetTransform();

        ImGui::TextUnformatted("Name");
        ImGui::TextUnformatted(selectedObject.GetName().c_str());

        ImGui::SeparatorText("Transform");
        engineGuideWindow_.DrawHelpButtonAtRight("feature-inspector");

        const DirectX::XMFLOAT3& currentPosition = transform.GetPosition();
        float position[] =
        {
            currentPosition.x,
            currentPosition.y,
            currentPosition.z
        };
        if (ImGui::DragFloat3("Position", position, 0.1f, 0.0f, 0.0f, "%.2f"))
        {
            transform.SetPosition(position[0], position[1], position[2]);
        }

        const DirectX::XMFLOAT3& currentRotation = transform.GetRotation();
        float rotationDegrees[] =
        {
            DirectX::XMConvertToDegrees(currentRotation.x),
            DirectX::XMConvertToDegrees(currentRotation.y),
            DirectX::XMConvertToDegrees(currentRotation.z)
        };
        if (ImGui::DragFloat3(
            "Rotation",
            rotationDegrees,
            1.0f,
            0.0f,
            0.0f,
            "%.1f"))
        {
            transform.SetRotation(
                DirectX::XMConvertToRadians(rotationDegrees[0]),
                DirectX::XMConvertToRadians(rotationDegrees[1]),
                DirectX::XMConvertToRadians(rotationDegrees[2]));
        }

        const DirectX::XMFLOAT3& currentScale = transform.GetScale();
        float scale[] =
        {
            currentScale.x,
            currentScale.y,
            currentScale.z
        };
        if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.0f, 0.0f, "%.2f"))
        {
            transform.SetScale(scale[0], scale[1], scale[2]);
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button(
                "Delete Object",
                ImVec2(-EngineGuideWindow::GetHelpButtonReservedWidth(), 0.0f)))
        {
            deleteObjectRequested_ = true;
        }
        engineGuideWindow_.DrawHelpButtonAtRight("feature-delete");
    }
    ImGui::End();

    // Viewport 조작과 창 제목의 통계는 Panel이 없으므로 화면 아래에 설명 Button만 모아 둡니다.
    ImGui::SetNextWindowPos(
        ImVec2(
            workPosition.x + (workSize.x - viewportHelpWidth) * 0.5f,
            workPosition.y + workSize.y - viewportHelpHeight - panelMargin),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(viewportHelpWidth, viewportHelpHeight),
        ImGuiCond_Always);
    ImGui::Begin(
        "Viewport Help",
        nullptr,
        panelFlags | ImGuiWindowFlags_NoTitleBar);
    ImGui::TextUnformatted("Look (RMB)");
    engineGuideWindow_.DrawHelpButton("feature-camera");
    ImGui::SameLine();
    ImGui::TextUnformatted("Pick (LMB)");
    engineGuideWindow_.DrawHelpButton("feature-picking");
    ImGui::SameLine();
    ImGui::TextUnformatted("Stats");
    engineGuideWindow_.DrawHelpButton("feature-stats");
    ImGui::SameLine();
    if (ImGui::SmallButton("Guide"))
    {
        engineGuideWindow_.OpenHome();
    }
    ImGui::End();

    engineGuideWindow_.Draw();
}

void D3D12Context::DrawModelPicker()
{
    const std::string selectedName =
        selectedModelFile_ < modelFiles_.size()
            ? modelFiles_[selectedModelFile_].stem().u8string()
            : std::string("No Models");

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::BeginCombo("##ModelFile", selectedName.c_str()))
    {
        // 목록을 열 때마다 다시 찾으므로 실행 중에 추가한 파일도 고를 수 있습니다.
        if (ImGui::IsWindowAppearing())
        {
            modelFiles_ = FindModelFiles();
        }
        for (std::size_t index = 0; index < modelFiles_.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::Selectable(
                    modelFiles_[index].stem().u8string().c_str(),
                    index == selectedModelFile_))
            {
                selectedModelFile_ = index;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(selectedModelFile_ >= modelFiles_.size());
    if (ImGui::Button("Add"))
    {
        const std::filesystem::path& file = modelFiles_[selectedModelFile_];
        if (const Model* const model = resourceManager_.LoadModel(file))
        {
            AddSceneObject(model, file.stem().u8string());
        }
    }
    ImGui::EndDisabled();
}

void D3D12Context::AddSceneObject(
    const Model* model,
    const std::string& baseName)
{
    if (model == nullptr)
    {
        return;
    }

    GameObject gameObject(model);
    gameObject.SetName(CreateUniqueObjectName(baseName));
    selectedObjectIndex_ = scene_.AddGameObject(std::move(gameObject));
}

std::string D3D12Context::CreateUniqueObjectName(
    const std::string& baseName) const
{
    const std::vector<GameObject>& gameObjects = scene_.GetGameObjects();
    const auto isNameUsed = [&gameObjects](const std::string& name)
    {
        return std::any_of(
            gameObjects.begin(),
            gameObjects.end(),
            [&name](const GameObject& gameObject)
            {
                return gameObject.GetName() == name;
            });
    };

    if (!isNameUsed(baseName))
    {
        return baseName;
    }

    for (std::size_t suffix = 1;
         suffix < (std::numeric_limits<std::size_t>::max)();
         ++suffix)
    {
        const std::string candidate =
            baseName + "_" + std::to_string(suffix);
        if (!isNameUsed(candidate))
        {
            return candidate;
        }
    }

    return baseName + "_New";
}

void D3D12Context::ProcessGizmoHotkeys()
{
    const ImGuiIO& io = ImGui::GetIO();
    if (isCameraLookActive_ ||
        Input::IsRightMouseButtonDown() ||
        io.WantCaptureKeyboard)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_W, false))
    {
        gizmoOperation_ = GizmoOperation::Translate;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_E, false))
    {
        gizmoOperation_ = GizmoOperation::Rotate;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_R, false))
    {
        gizmoOperation_ = GizmoOperation::Scale;
    }
}

void D3D12Context::DrawTransformGizmo()
{
    std::vector<GameObject>& gameObjects = scene_.GetGameObjects();
    if (selectedObjectIndex_ == InvalidObjectIndex ||
        selectedObjectIndex_ >= gameObjects.size())
    {
        ImGuizmo::Enable(false);
        return;
    }

    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
    {
        ImGuizmo::Enable(false);
        return;
    }

    // RMB Camera Mode에서는 Gizmo를 표시하되 입력은 받지 않도록 합니다.
    ImGuizmo::Enable(
        !isCameraLookActive_ && !Input::IsRightMouseButtonDown());
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(
        viewport.TopLeftX,
        viewport.TopLeftY,
        viewport.Width,
        viewport.Height);

    Transform& transform =
        gameObjects[selectedObjectIndex_].GetTransform();

    DirectX::XMFLOAT4X4 viewMatrix{};
    DirectX::XMFLOAT4X4 projectionMatrix{};
    DirectX::XMFLOAT4X4 objectMatrix{};
    DirectX::XMStoreFloat4x4(&viewMatrix, camera_.GetViewMatrix());
    DirectX::XMStoreFloat4x4(&projectionMatrix, GetProjectionMatrix());
    DirectX::XMStoreFloat4x4(&objectMatrix, transform.GetWorldMatrix());

    // DirectXMath의 CPU 행 벡터 Matrix 원본은 ImGuizmo의 메모리 배치와 직접 대응합니다.
    // GPU Constant Buffer용으로 전치한 Matrix는 이 경로에서 사용하지 않습니다.
    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    switch (gizmoOperation_)
    {
    case GizmoOperation::Translate:
        operation = ImGuizmo::TRANSLATE;
        break;
    case GizmoOperation::Rotate:
        operation = ImGuizmo::ROTATE;
        break;
    case GizmoOperation::Scale:
        operation = ImGuizmo::SCALE;
        break;
    }

    // Translate/Rotate는 World, Scale은 ImGuizmo 특성에 맞춰 Local Mode를 사용합니다.
    const ImGuizmo::MODE mode = operation == ImGuizmo::SCALE
        ? ImGuizmo::LOCAL
        : ImGuizmo::WORLD;
    const bool wasModified = ImGuizmo::Manipulate(
        &viewMatrix.m[0][0],
        &projectionMatrix.m[0][0],
        operation,
        mode,
        &objectMatrix.m[0][0]);
    if (!wasModified)
    {
        return;
    }

    float translation[3]{};
    float rotationDegrees[3]{};
    float scale[3]{};
    ImGuizmo::DecomposeMatrixToComponents(
        &objectMatrix.m[0][0],
        translation,
        rotationDegrees,
        scale);

    transform.SetPosition(
        translation[0],
        translation[1],
        translation[2]);
    transform.SetRotation(
        DirectX::XMConvertToRadians(rotationDegrees[0]),
        DirectX::XMConvertToRadians(rotationDegrees[1]),
        DirectX::XMConvertToRadians(rotationDegrees[2]));
    transform.SetScale(scale[0], scale[1], scale[2]);
}

void D3D12Context::ProcessObjectDeletionRequest()
{
    if (!deleteObjectRequested_)
    {
        return;
    }

    deleteObjectRequested_ = false;
    if (selectedObjectIndex_ == InvalidObjectIndex)
    {
        return;
    }

    scene_.RemoveGameObject(selectedObjectIndex_);
    selectedObjectIndex_ = InvalidObjectIndex;
}

DirectX::XMMATRIX D3D12Context::GetProjectionMatrix() const
{
    const float aspectRatio =
        static_cast<float>(clientWidth_) / static_cast<float>(clientHeight_);
    return DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        aspectRatio,
        0.1f,
        100.0f);
}

bool D3D12Context::CreatePickingRay(
    const POINT& mousePosition,
    Ray& ray) const
{
    const D3D12_VIEWPORT& viewport = swapChain_.GetViewport();
    if (viewport.Width <= 0.0f || viewport.Height <= 0.0f ||
        clientWidth_ == 0 || clientHeight_ == 0)
    {
        return false;
    }

    const float mouseX = static_cast<float>(mousePosition.x);
    const float mouseY = static_cast<float>(mousePosition.y);
    if (mouseX < viewport.TopLeftX ||
        mouseX >= viewport.TopLeftX + viewport.Width ||
        mouseY < viewport.TopLeftY ||
        mouseY >= viewport.TopLeftY + viewport.Height)
    {
        return false;
    }

    const DirectX::XMMATRIX viewMatrix = camera_.GetViewMatrix();
    const DirectX::XMMATRIX projectionMatrix = GetProjectionMatrix();
    const DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();
    const DirectX::XMVECTOR screenNearPoint = DirectX::XMVectorSet(
        mouseX,
        mouseY,
        viewport.MinDepth,
        1.0f);
    const DirectX::XMVECTOR screenFarPoint = DirectX::XMVectorSet(
        mouseX,
        mouseY,
        viewport.MaxDepth,
        1.0f);

    const DirectX::XMVECTOR worldNearPoint = DirectX::XMVector3Unproject(
        screenNearPoint,
        viewport.TopLeftX,
        viewport.TopLeftY,
        viewport.Width,
        viewport.Height,
        viewport.MinDepth,
        viewport.MaxDepth,
        projectionMatrix,
        viewMatrix,
        worldMatrix);
    const DirectX::XMVECTOR worldFarPoint = DirectX::XMVector3Unproject(
        screenFarPoint,
        viewport.TopLeftX,
        viewport.TopLeftY,
        viewport.Width,
        viewport.Height,
        viewport.MinDepth,
        viewport.MaxDepth,
        projectionMatrix,
        viewMatrix,
        worldMatrix);
    const DirectX::XMVECTOR rayDirection = DirectX::XMVectorSubtract(
        worldFarPoint,
        worldNearPoint);
    const float directionLengthSquared = DirectX::XMVectorGetX(
        DirectX::XMVector3LengthSq(rayDirection));
    if (!std::isfinite(directionLengthSquared) ||
        directionLengthSquared <= 1.0e-12f)
    {
        return false;
    }

    ray.origin = camera_.GetPosition();
    DirectX::XMStoreFloat3(
        &ray.direction,
        DirectX::XMVector3Normalize(rayDirection));
    return true;
}

void D3D12Context::PerformViewportPicking(const POINT& mousePosition)
{
    Ray ray{};
    if (!CreatePickingRay(mousePosition, ray))
    {
        return;
    }

    std::size_t closestObjectIndex = InvalidObjectIndex;
    float closestDistance = (std::numeric_limits<float>::max)();
    const std::vector<GameObject>& gameObjects = scene_.GetGameObjects();
    for (std::size_t objectIndex = 0;
         objectIndex < gameObjects.size();
         ++objectIndex)
    {
        const GameObject& gameObject = gameObjects[objectIndex];
        if (gameObject.GetModel() == nullptr)
        {
            continue;
        }

        float intersectionDistance = 0.0f;
        if (IntersectRayAABB(
                ray,
                gameObject.GetWorldBounds(),
                intersectionDistance) &&
            intersectionDistance < closestDistance)
        {
            closestDistance = intersectionDistance;
            closestObjectIndex = objectIndex;
        }
    }

    // Hit가 없으면 기존 Selection도 해제하여 Hierarchy와 Inspector가 같은 상태를 공유합니다.
    selectedObjectIndex_ = closestObjectIndex;
}

void D3D12Context::AllocateImGuiDescriptor(
    ImGui_ImplDX12_InitInfo* information,
    D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
{
    D3D12Context* const context = information != nullptr
        ? static_cast<D3D12Context*>(information->UserData)
        : nullptr;
    IM_ASSERT(context != nullptr);
    IM_ASSERT(cpuHandle != nullptr && gpuHandle != nullptr);
    IM_ASSERT(
        context != nullptr &&
        !context->imguiFreeDescriptorIndices_.empty());
    if (context == nullptr || cpuHandle == nullptr || gpuHandle == nullptr ||
        context->imguiFreeDescriptorIndices_.empty())
    {
        return;
    }

    const UINT descriptorIndex = context->imguiFreeDescriptorIndices_.back();
    context->imguiFreeDescriptorIndices_.pop_back();

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuStart =
        context->imguiDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE gpuStart =
        context->imguiDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    cpuHandle->ptr = cpuStart.ptr +
        static_cast<SIZE_T>(descriptorIndex) * context->imguiDescriptorSize_;
    gpuHandle->ptr = gpuStart.ptr +
        static_cast<UINT64>(descriptorIndex) * context->imguiDescriptorSize_;
}

void D3D12Context::FreeImGuiDescriptor(
    ImGui_ImplDX12_InitInfo* information,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    D3D12Context* const context = information != nullptr
        ? static_cast<D3D12Context*>(information->UserData)
        : nullptr;
    IM_ASSERT(context != nullptr);
    if (context == nullptr || context->imguiDescriptorSize_ == 0)
    {
        return;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuStart =
        context->imguiDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    const D3D12_GPU_DESCRIPTOR_HANDLE gpuStart =
        context->imguiDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    const UINT cpuIndex = static_cast<UINT>(
        (cpuHandle.ptr - cpuStart.ptr) / context->imguiDescriptorSize_);
    const UINT gpuIndex = static_cast<UINT>(
        (gpuHandle.ptr - gpuStart.ptr) / context->imguiDescriptorSize_);

    IM_ASSERT(cpuIndex == gpuIndex);
    IM_ASSERT(cpuIndex < ImGuiDescriptorCount);
    if (cpuIndex == gpuIndex && cpuIndex < ImGuiDescriptorCount)
    {
        context->imguiFreeDescriptorIndices_.push_back(cpuIndex);
    }
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

bool D3D12Context::CreatePrimitiveModels()
{
    modelFiles_ = FindModelFiles();

    return resourceManager_.AddModel("Cube", CreateCubeGeometry()) != nullptr &&
        resourceManager_.AddModel("Sphere", CreateSphereGeometry()) != nullptr;
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

    // 빈 Scene에서는 Object Buffer 생성을 미루고 첫 Object가 추가된 Frame에 기존 Capacity 경로로 생성합니다.
    return EnsureConstantBufferCapacity(scene_.GetGameObjectCount());
}

bool D3D12Context::EnsureConstantBufferCapacity(
    std::size_t requiredCapacity)
{
    if (requiredCapacity <= objectConstantBufferCapacity_)
    {
        return true;
    }

    const UINT64 bytesPerObjectCapacity =
        static_cast<UINT64>(AlignedConstantBufferSize) *
        D3D12Config::FrameCount;
    const std::size_t maximumCapacity = static_cast<std::size_t>(
        (std::numeric_limits<UINT64>::max)() / bytesPerObjectCapacity);
    if (requiredCapacity > maximumCapacity)
    {
        std::fprintf(stderr, "Object Constant Buffer Capacity가 너무 큽니다.\n");
        return false;
    }

    std::size_t newCapacity = (std::max)(
        objectConstantBufferCapacity_,
        static_cast<std::size_t>(1));
    while (newCapacity < requiredCapacity)
    {
        if (newCapacity > maximumCapacity / 2)
        {
            newCapacity = requiredCapacity;
            break;
        }

        newCapacity *= 2;
    }

    return RecreateConstantBuffer(newCapacity);
}

bool D3D12Context::RecreateConstantBuffer(std::size_t newCapacity)
{
    if (newCapacity == 0)
    {
        return false;
    }

    // 기존 Resource를 교체하기 전에 Queue에 제출된 모든 참조가 끝났는지 Fence로 확인합니다.
    if (constantBuffer_ != nullptr && !commandContext_.WaitForGPU())
    {
        return false;
    }

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
    // Frame별 stride를 Capacity로 고정하여 실행 중 Object Count가 바뀌어도 영역이 겹치지 않습니다.
    resourceDescription.Width =
        static_cast<UINT64>(AlignedConstantBufferSize) *
        D3D12Config::FrameCount *
        newCapacity;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.SampleDesc.Quality = 0;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12Resource> newConstantBuffer;
    const HRESULT resourceResult = deviceResources_.GetDevice()->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&newConstantBuffer));

    if (FAILED(resourceResult))
    {
        std::fprintf(
            stderr,
            "상수 버퍼용 ID3D12Device::CreateCommittedResource 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(resourceResult));
        return false;
    }

    // 한 번 얻은 CPU 주소를 계속 보관하는 Persistent Mapping 방식으로 사용합니다.
    void* newMappedData = nullptr;
    const D3D12_RANGE readRange{ 0, 0 };
    const HRESULT mapResult = newConstantBuffer->Map(
        0,
        &readRange,
        &newMappedData);

    if (FAILED(mapResult))
    {
        std::fprintf(
            stderr,
            "상수 버퍼의 ID3D12Resource::Map 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(mapResult));
        return false;
    }

    std::memset(
        newMappedData,
        0,
        static_cast<SIZE_T>(resourceDescription.Width));

    if (constantBuffer_ != nullptr && constantBufferMappedData_ != nullptr)
    {
        constantBuffer_->Unmap(0, nullptr);
    }

    constantBuffer_ = std::move(newConstantBuffer);
    constantBufferMappedData_ = newMappedData;
    objectConstantBufferCapacity_ = newCapacity;

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
        static_cast<std::size_t>(frameIndex) *
        objectConstantBufferCapacity_ +
        objectIndex;
    unsigned char* const objectData =
        static_cast<unsigned char*>(constantBufferMappedData_) +
        static_cast<SIZE_T>(constantBufferSlot) * AlignedConstantBufferSize;
    std::memset(objectData, 0, AlignedConstantBufferSize);
    std::memcpy(
        objectData,
        &constantBufferData,
        sizeof(constantBufferData));
}
