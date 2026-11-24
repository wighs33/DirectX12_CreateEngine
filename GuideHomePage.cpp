#include "GuideHomePage.h"

#include "EngineGuide.h"
#include "GuideCardGrid.h"
#include "GuideFeatureButtons.h"
#include "GuideTheme.h"

#include "imgui.h"

#include <vector>

void GuideHomePage::Draw(const EngineGuide& guide, GuideNavigation& navigation)
{
    GuideTheme::DrawMutedText(
        "클래스 하나가 카드 한 장입니다. 기능을 누르면 어떤 카드를 거쳐 움직이는지 볼 수 있습니다.");
    ImGui::Spacing();

    GuideTheme::DrawHeading("기능", GuideTheme::HeadingFontSize);
    std::vector<const GuideFeature*> features;
    for (const GuideFeature& feature : guide.GetFeatures())
    {
        features.push_back(&feature);
    }
    GuideFeatureButtons::Draw(features, navigation);

    for (const GuideGroup& group : guide.GetGroups())
    {
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        GuideTheme::DrawHeading(group.name.c_str(), GuideTheme::HeadingFontSize);
        GuideCardGrid::Draw(group.cardIds, guide, navigation);
    }
}
