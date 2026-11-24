#include "CreatePanel.h"

#include "EditorLayout.h"
#include "EditorSelection.h"
#include "GameObject.h"
#include "GuideHelpButton.h"
#include "Model.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "SceneSerializer.h"

#include "imgui.h"

#include <algorithm>
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

void CreatePanel::Initialize()
{
    modelFiles_ = FindModelFiles();
}

void CreatePanel::Draw(
    Scene& scene,
    ResourceManager& resourceManager,
    EditorSelection& selection,
    EngineGuideWindow& guideWindow)
{
    EditorLayout::BeginPanel("Create", EditorLayout::GetCreatePanel());
    if (ImGui::Button("Cube", ImVec2(120.0f, 0.0f)))
    {
        AddObject(resourceManager.FindModel("Cube"), "Cube", scene, selection);
    }
    GuideHelpButton::DrawAtRight(guideWindow, "feature-create");
    if (ImGui::Button("Sphere", ImVec2(120.0f, 0.0f)))
    {
        AddObject(resourceManager.FindModel("Sphere"), "Sphere", scene, selection);
    }
    if (ImGui::Button("Plane", ImVec2(120.0f, 0.0f)))
    {
        AddObject(resourceManager.FindModel("Plane"), "Plane", scene, selection);
    }
    DrawModelPicker(scene, resourceManager, selection);
    GuideHelpButton::DrawAtRight(guideWindow, "feature-model");

    ImGui::Separator();
    DrawSceneButtons(scene, resourceManager, selection);
    GuideHelpButton::DrawAtRight(guideWindow, "feature-save");
    ImGui::End();
}

void CreatePanel::DrawModelPicker(
    Scene& scene,
    ResourceManager& resourceManager,
    EditorSelection& selection)
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
        if (const Model* const model = resourceManager.LoadModel(file))
        {
            AddObject(model, file.stem().u8string(), scene, selection);
        }
    }
    ImGui::EndDisabled();
}

void CreatePanel::DrawSceneButtons(
    Scene& scene,
    ResourceManager& resourceManager,
    EditorSelection& selection)
{
    if (ImGui::Button("Save Scene", ImVec2(84.0f, 0.0f)))
    {
        SceneSerializer::SaveScene(scene, GetSceneFilePath());
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene", ImVec2(84.0f, 0.0f)) &&
        SceneSerializer::LoadScene(
            scene,
            resourceManager,
            GetSceneFilePath()))
    {
        selection.Clear();
    }
}

void CreatePanel::AddObject(
    const Model* model,
    const std::string& baseName,
    Scene& scene,
    EditorSelection& selection)
{
    if (model == nullptr)
    {
        return;
    }

    GameObject gameObject(model);
    gameObject.SetName(scene.CreateUniqueName(baseName));
    selection.Select(scene.AddGameObject(std::move(gameObject)));
}
