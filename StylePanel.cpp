#include "StylePanel.h"

#include "EditorLayout.h"
#include "EditorSelection.h"
#include "GameObject.h"
#include "GuideHelpButton.h"
#include "MaterialStyleLibrary.h"
#include "Model.h"
#include "Scene.h"

#include "imgui.h"

#include <algorithm>

namespace
{
    constexpr float ValueWidth = 150.0f;

    // 직접 입력한 값도 범위를 벗어나지 않게 합니다.
    void DrawSlider(
        const char* label,
        float& value,
        float minimum,
        float maximum,
        const char* format = "%.3f")
    {
        ImGui::SliderFloat(
            label,
            &value,
            minimum,
            maximum,
            format,
            ImGuiSliderFlags_AlwaysClamp);
    }

    const char* GetLabel(const std::string& materialName)
    {
        return materialName.empty() ? "(No Name)" : materialName.c_str();
    }
}

void StylePanel::Draw(
    Scene& scene,
    const EditorSelection& selection,
    MaterialStyleLibrary& styles,
    EngineGuideWindow& guideWindow)
{
    EditorLayout::BeginPanel("Style", EditorLayout::GetStylePanel());
    const GameObject* const selectedObject = selection.GetSelected(scene);
    const Model* const model =
        selectedObject != nullptr ? selectedObject->GetModel() : nullptr;

    if (model == nullptr)
    {
        ImGui::TextDisabled("Select an object");
        GuideHelpButton::DrawAtRight(guideWindow, "feature-style");
    }
    else
    {
        ImGui::PushItemWidth(ValueWidth);
        DrawMaterialPicker(GetMaterialNames(*model));
        GuideHelpButton::DrawAtRight(guideWindow, "feature-style");

        MaterialStyle& style = styles.FindOrAdd(materialName_);
        // 재질마다 다른 ID를 주어, 끌던 값이 다른 재질로 넘어가지 않게 합니다.
        ImGui::PushID(materialName_.c_str());
        DrawShade(style);
        DrawRim(style);
        DrawOutline(style);
        ImGui::PopID();
        ImGui::PopItemWidth();
    }

    ImGui::Spacing();
    ImGui::Separator();
    DrawFileButtons(styles);
    ImGui::End();
}

std::vector<std::string> StylePanel::GetMaterialNames(const Model& model)
{
    std::vector<std::string> names;
    for (const Material& material : model.GetMaterials())
    {
        if (std::find(names.begin(), names.end(), material.GetName()) ==
            names.end())
        {
            names.push_back(material.GetName());
        }
    }
    return names;
}

void StylePanel::DrawMaterialPicker(
    const std::vector<std::string>& materialNames)
{
    if (std::find(
            materialNames.begin(),
            materialNames.end(),
            materialName_) == materialNames.end())
    {
        materialName_ =
            materialNames.empty() ? std::string() : materialNames.front();
    }

    if (ImGui::BeginCombo("##Material", GetLabel(materialName_)))
    {
        for (std::size_t index = 0; index < materialNames.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::Selectable(
                    GetLabel(materialNames[index]),
                    materialNames[index] == materialName_))
            {
                materialName_ = materialNames[index];
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
}

void StylePanel::DrawShade(MaterialStyle& style)
{
    ImGui::SeparatorText("Shade");
    ImGui::PushID("Shade");
    ImGui::ColorEdit3("Color", &style.shadeColor.x);
    DrawSlider("Threshold", style.shadeThreshold, -1.0f, 1.0f);
    DrawSlider("Softness", style.shadeSoftness, 0.001f, 0.5f);
    DrawSlider("Ambient", style.ambientWeight, 0.0f, 1.0f);
    ImGui::PopID();
}

void StylePanel::DrawRim(MaterialStyle& style)
{
    ImGui::SeparatorText("Rim");
    ImGui::PushID("Rim");
    ImGui::ColorEdit3("Color", &style.rimColor.x);
    DrawSlider("Strength", style.rimStrength, 0.0f, 1.0f);
    DrawSlider("Width", style.rimWidth, 0.0f, 1.0f);
    ImGui::PopID();
}

void StylePanel::DrawOutline(MaterialStyle& style)
{
    ImGui::SeparatorText("Outline");
    ImGui::PushID("Outline");
    ImGui::ColorEdit3("Color", &style.outlineColor.x);
    DrawSlider("Width (px)", style.outlineWidth, 0.0f, 8.0f, "%.2f");
    ImGui::PopID();
}

void StylePanel::DrawFileButtons(MaterialStyleLibrary& styles)
{
    const bool isModified = styles.IsModified();

    ImGui::BeginDisabled(!isModified);
    if (ImGui::Button("Save Style"))
    {
        styles.Save();
    }
    ImGui::EndDisabled();

    // 파일을 밖에서 고쳤을 수도 있으므로 고친 값이 없어도 누를 수 있습니다.
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        styles.Reload();
    }
    if (isModified)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Not saved");
    }
}
