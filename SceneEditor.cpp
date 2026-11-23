#include "SceneEditor.h"

#include "Camera.h"
#include "GameObject.h"
#include "Input.h"
#include "Model.h"
#include "Ray.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SceneSerializer.h"

#include "imgui.h"
#include "ImGuizmo.h"

#include <algorithm>
#include <cmath>
#include <system_error>
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

void SceneEditor::Initialize(
    HWND window,
    Scene& scene,
    Camera& camera,
    ResourceManager& resourceManager)
{
    window_ = window;
    scene_ = &scene;
    camera_ = &camera;
    resourceManager_ = &resourceManager;

    engineGuideWindow_.Initialize(GetGuideFilePath());
    modelFiles_ = FindModelFiles();
}

void SceneEditor::Update(float deltaTime, const D3D12_VIEWPORT& viewport)
{
    viewport_ = viewport;

    Input::Update(window_);
    UpdateCameraLookState();
    ProcessDeleteHotkey();

    // Hotkey와 UI 편집을 처리한 뒤 삭제로 Selection을 정리하고 마지막에 Gizmo를 갱신합니다.
    ProcessGizmoHotkeys();
    DrawEditorUI();
    ProcessObjectDeletionRequest();
    DrawTransformGizmo();

    UpdateCamera(deltaTime);
    ProcessPicking();
}

void SceneEditor::UpdateCameraLookState()
{
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
}

void SceneEditor::ProcessDeleteHotkey()
{
    const ImGuiIO& io = ImGui::GetIO();

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
}

void SceneEditor::UpdateCamera(float deltaTime)
{
    constexpr float cameraSpeed = 3.0f;
    constexpr float mouseSensitivity = 0.002f;

    const ImGuiIO& io = ImGui::GetIO();
    const bool isGizmoUsing = ImGuizmo::IsUsing();

    // Mouse Delta는 이미 한 Frame 동안 이동한 Pixel 양이므로 DeltaTime을 곱하지 않습니다.
    if (isCameraLookActive_ && !isGizmoUsing)
    {
        const MouseDelta mouseDelta = Input::GetMouseDelta();
        camera_->Rotate(
            static_cast<float>(mouseDelta.x) * mouseSensitivity,
            static_cast<float>(-mouseDelta.y) * mouseSensitivity);
    }

    const float movementDistance = cameraSpeed * deltaTime;

    if (isCameraLookActive_ &&
        !isGizmoUsing &&
        !io.WantCaptureKeyboard)
    {
        if (Input::IsKeyDown('W'))
            camera_->MoveForward(movementDistance);
        if (Input::IsKeyDown('S'))
            camera_->MoveForward(-movementDistance);
        if (Input::IsKeyDown('A'))
            camera_->MoveRight(-movementDistance);
        if (Input::IsKeyDown('D'))
            camera_->MoveRight(movementDistance);
    }
}

void SceneEditor::ProcessPicking()
{
    const ImGuiIO& io = ImGui::GetIO();
    const std::vector<GameObject>& gameObjects = scene_->GetGameObjects();
    const bool hasValidSelection =
        selectedObjectIndex_ != InvalidObjectIndex &&
        selectedObjectIndex_ < gameObjects.size();
    const bool isGizmoInteraction =
        hasValidSelection &&
        (ImGuizmo::IsOver() || ImGuizmo::IsUsing());

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

void SceneEditor::DrawEditorUI()
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

    std::vector<GameObject>& gameObjects = scene_->GetGameObjects();
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
        AddSceneObject(resourceManager_->FindModel("Cube"), "Cube");
    }
    engineGuideWindow_.DrawHelpButtonAtRight("feature-create");
    if (ImGui::Button("Sphere", ImVec2(120.0f, 0.0f)))
    {
        AddSceneObject(resourceManager_->FindModel("Sphere"), "Sphere");
    }
    DrawModelPicker();
    engineGuideWindow_.DrawHelpButtonAtRight("feature-model");

    ImGui::Separator();
    if (ImGui::Button("Save Scene", ImVec2(84.0f, 0.0f)))
    {
        SceneSerializer::SaveScene(*scene_, GetSceneFilePath());
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene", ImVec2(84.0f, 0.0f)))
    {
        if (SceneSerializer::LoadScene(
                *scene_,
                *resourceManager_,
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

void SceneEditor::DrawModelPicker()
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
        if (const Model* const model = resourceManager_->LoadModel(file))
        {
            AddSceneObject(model, file.stem().u8string());
        }
    }
    ImGui::EndDisabled();
}

void SceneEditor::AddSceneObject(
    const Model* model,
    const std::string& baseName)
{
    if (model == nullptr)
    {
        return;
    }

    GameObject gameObject(model);
    gameObject.SetName(CreateUniqueObjectName(baseName));
    selectedObjectIndex_ = scene_->AddGameObject(std::move(gameObject));
}

std::string SceneEditor::CreateUniqueObjectName(
    const std::string& baseName) const
{
    const std::vector<GameObject>& gameObjects = scene_->GetGameObjects();
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

void SceneEditor::ProcessGizmoHotkeys()
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

void SceneEditor::DrawTransformGizmo()
{
    std::vector<GameObject>& gameObjects = scene_->GetGameObjects();
    if (selectedObjectIndex_ == InvalidObjectIndex ||
        selectedObjectIndex_ >= gameObjects.size())
    {
        ImGuizmo::Enable(false);
        return;
    }

    const D3D12_VIEWPORT& viewport = viewport_;
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
    DirectX::XMStoreFloat4x4(&viewMatrix, camera_->GetViewMatrix());
    DirectX::XMStoreFloat4x4(&projectionMatrix, camera_->GetProjectionMatrix());
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

void SceneEditor::ProcessObjectDeletionRequest()
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

    scene_->RemoveGameObject(selectedObjectIndex_);
    selectedObjectIndex_ = InvalidObjectIndex;
}

bool SceneEditor::CreatePickingRay(
    const POINT& mousePosition,
    Ray& ray) const
{
    const D3D12_VIEWPORT& viewport = viewport_;
    if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
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

    const DirectX::XMMATRIX viewMatrix = camera_->GetViewMatrix();
    const DirectX::XMMATRIX projectionMatrix = camera_->GetProjectionMatrix();
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

    ray.origin = camera_->GetPosition();
    DirectX::XMStoreFloat3(
        &ray.direction,
        DirectX::XMVector3Normalize(rayDirection));
    return true;
}

void SceneEditor::PerformViewportPicking(const POINT& mousePosition)
{
    Ray ray{};
    if (!CreatePickingRay(mousePosition, ray))
    {
        return;
    }

    std::size_t closestObjectIndex = InvalidObjectIndex;
    float closestDistance = (std::numeric_limits<float>::max)();
    const std::vector<GameObject>& gameObjects = scene_->GetGameObjects();
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
