#pragma once

#include <string>
#include <vector>

class EditorSelection;
class EngineGuideWindow;
class MaterialStyleLibrary;
class Model;
class Scene;
struct MaterialStyle;

// 고른 물체의 재질마다 카툰 그림체 값을 고치고 파일에 저장하는 패널입니다.
class StylePanel final
{
public:
    // 값을 고치는 동안 화면이 바로 바뀝니다.
    void Draw(
        Scene& scene,
        const EditorSelection& selection,
        MaterialStyleLibrary& styles,
        EngineGuideWindow& guideWindow);

private:
    // 모델이 쓰는 재질의 이름을 한 번씩만 적습니다.
    static std::vector<std::string> GetMaterialNames(const Model& model);

    void DrawMaterialPicker(const std::vector<std::string>& materialNames);
    static void DrawShade(MaterialStyle& style);
    static void DrawRim(MaterialStyle& style);
    static void DrawOutline(MaterialStyle& style);
    static void DrawFileButtons(MaterialStyleLibrary& styles);

    // 다른 물체를 골라도 같은 이름의 재질이 있으면 계속 보여 줍니다.
    std::string materialName_;
};
