#include "EngineGuideWindow.h"

#include "GuidePictureRenderer.h"

#include "imgui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <string>

void EngineGuideWindow::Initialize(const std::filesystem::path& guideFilePath)
{
    theme_.LoadFont();
    isLoaded_ = guide_.Load(guideFilePath);
}

void EngineGuideWindow::DrawHelpButton(const char* id)
{
    ImGui::SameLine();
    DrawHelpButtonOnCurrentLine(id);
}

void EngineGuideWindow::DrawHelpButtonAtRight(const char* id)
{
    // 남은 폭을 기준으로 계산하므로 Scrollbar가 생겨도 물음표가 가려지지 않습니다.
    ImGui::SameLine();
    const float buttonWidth =
        GetHelpButtonReservedWidth() - ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        ImGui::GetContentRegionAvail().x -
        buttonWidth);
    DrawHelpButtonOnCurrentLine(id);
}

float EngineGuideWindow::GetHelpButtonReservedWidth()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    return ImGui::CalcTextSize("?").x +
        style.FramePadding.x * 2.0f +
        style.ItemSpacing.x;
}

void EngineGuideWindow::DrawHelpButtonOnCurrentLine(const char* id)
{
    // 글자가 모두 '?'로 같으므로 설명 이름을 ID로 사용해 Button을 구분합니다.
    ImGui::PushID(id);
    if (ImGui::SmallButton("?"))
    {
        Open(id);
    }
    ImGui::PopID();
}

void EngineGuideWindow::OpenHome()
{
    Open(std::string());
}

void EngineGuideWindow::Open(const std::string& id)
{
    isOpen_ = true;
    isFocusRequested_ = true;
    navigation_.Request(id);
}

void EngineGuideWindow::Draw()
{
    if (!isOpen_)
    {
        return;
    }

    isScrollResetRequested_ = navigation_.Apply() || isScrollResetRequested_;

    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        viewport->GetWorkCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(
        ImVec2(
            (std::min)(1180.0f, viewport->WorkSize.x - 40.0f),
            (std::min)(640.0f, viewport->WorkSize.y - 40.0f)),
        ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(380.0f, 300.0f),
        ImVec2(FLT_MAX, FLT_MAX));
    if (isFocusRequested_)
    {
        ImGui::SetNextWindowFocus();
        isFocusRequested_ = false;
    }

    theme_.Push();

    // '###' 뒤의 이름만 창을 구분하므로 보이는 제목은 자유롭게 바꿀 수 있습니다.
    if (ImGui::Begin(
            "Pandora Engine 도감###EngineGuide",
            &isOpen_,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
    {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            isOpen_ = false;
        }

        DrawNavigationBar();
        ImGui::Separator();

        if (ImGui::BeginChild("GuideContent"))
        {
            if (isScrollResetRequested_)
            {
                ImGui::SetScrollY(0.0f);
                isScrollResetRequested_ = false;
            }

            const GuideCard* const card = guide_.FindCard(navigation_.GetCurrentId());
            const GuideFeature* const feature = guide_.FindFeature(navigation_.GetCurrentId());
            if (!isLoaded_)
            {
                ImGui::TextWrapped(
                    "설명 파일을 읽지 못했습니다. Assets/Guide/EngineGuide.json을 확인하세요.");
            }
            else if (card != nullptr)
            {
                DrawCardPage(*card);
            }
            else if (feature != nullptr)
            {
                DrawFeaturePage(*feature);
            }
            else
            {
                DrawHome();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();

    theme_.Pop();
}

void EngineGuideWindow::DrawNavigationBar()
{
    ImGui::BeginDisabled(!navigation_.CanGoBack());
    if (ImGui::Button("← 뒤로"))
    {
        navigation_.RequestBack();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(navigation_.GetCurrentId().empty());
    if (ImGui::Button("전체 카드"))
    {
        navigation_.Request(std::string());
    }
    ImGui::EndDisabled();

    const std::vector<GuideCard>& cards = guide_.GetCards();
    const auto current = std::find_if(
        cards.begin(),
        cards.end(),
        [this](const GuideCard& card) { return card.id == navigation_.GetCurrentId(); });
    if (current == cards.end())
    {
        return;
    }

    if (current != cards.begin())
    {
        const GuideCard& previous = *(current - 1);
        ImGui::SameLine();
        if (ImGui::Button(("이전: " + previous.name + "###Previous").c_str()))
        {
            navigation_.Request(previous.id);
        }
    }
    if (current + 1 != cards.end())
    {
        const GuideCard& next = *(current + 1);
        ImGui::SameLine();
        if (ImGui::Button(("다음: " + next.name + "###Next").c_str()))
        {
            navigation_.Request(next.id);
        }
    }
}

void EngineGuideWindow::DrawHome()
{
    GuideTheme::DrawMutedText(
        "클래스 하나가 카드 한 장입니다. 기능을 누르면 어떤 카드를 거쳐 움직이는지 볼 수 있습니다.");
    ImGui::Spacing();

    GuideTheme::DrawHeading("기능", GuideTheme::HeadingFontSize);
    std::vector<const GuideFeature*> features;
    for (const GuideFeature& feature : guide_.GetFeatures())
    {
        features.push_back(&feature);
    }
    DrawFeatureButtons(features);

    for (const GuideGroup& group : guide_.GetGroups())
    {
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        GuideTheme::DrawHeading(group.name.c_str(), GuideTheme::HeadingFontSize);
        DrawCardGrid(group.cardIds);
    }
}

void EngineGuideWindow::DrawCardPage(const GuideCard& card)
{
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float pictureWidth = (std::min)(440.0f, availableWidth * 0.5f);
    const ImVec2 picturePosition = ImGui::GetCursorScreenPos();
    GuidePictureRenderer::Draw(
        card,
        { picturePosition.x, picturePosition.y },
        pictureWidth);
    ImGui::Dummy(ImVec2(
        pictureWidth,
        GuidePictureRenderer::GetHeight(pictureWidth)));

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::BeginGroup();
    GuideTheme::DrawHeading(card.name.c_str(), GuideTheme::TitleFontSize);
    ImGui::TextWrapped("%s", card.role.c_str());
    GuideTheme::DrawMutedText(card.groupName + " · " + card.file);
    ImGui::EndGroup();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    GuideTheme::DrawSection("없으면 어떻게 될까", card.without);
    GuideTheme::DrawSection("지금의 한계", card.limit);
    GuideTheme::DrawSection("언리얼에서는", card.unreal);

    GuideTheme::DrawHeading("하는 일", GuideTheme::HeadingFontSize);
    if (ImGui::BeginTable(
            "Functions",
            2,
            ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("함수나 코드", 0, 0.38f);
        ImGui::TableSetupColumn("설명", 0, 0.62f);
        ImGui::TableHeadersRow();
        for (const GuideFunction& function : card.functions)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", function.name.c_str());
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", function.description.c_str());
        }
        ImGui::EndTable();
    }

    std::vector<const GuideFeature*> features;
    for (const GuideFeature& feature : guide_.GetFeatures())
    {
        const bool usesCard = std::any_of(
            feature.steps.begin(),
            feature.steps.end(),
            [&card](const GuideStep& step) { return step.cardId == card.id; });
        if (usesCard)
        {
            features.push_back(&feature);
        }
    }
    if (!features.empty())
    {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        GuideTheme::DrawHeading("이 카드를 쓰는 기능", GuideTheme::HeadingFontSize);
        DrawFeatureButtons(features);
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    GuideTheme::DrawHeading("이어진 카드", GuideTheme::HeadingFontSize);
    DrawCardGrid(card.links);
}

void EngineGuideWindow::DrawFeaturePage(const GuideFeature& feature)
{
    GuideTheme::DrawHeading(feature.name.c_str(), GuideTheme::TitleFontSize);
    ImGui::TextWrapped("%s", feature.howTo.c_str());
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    GuideTheme::DrawHeading("움직이는 순서", GuideTheme::HeadingFontSize);

    constexpr float circleRadius = 15.0f;
    constexpr float textIndent = 46.0f;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    std::vector<std::string> cardIds;
    for (std::size_t index = 0; index < feature.steps.size(); ++index)
    {
        const GuideStep& step = feature.steps[index];
        const GuideCard* const card = guide_.FindCard(step.cardId);
        if (card == nullptr)
        {
            continue;
        }
        if (std::find(cardIds.begin(), cardIds.end(), card->id) ==
            cardIds.end())
        {
            cardIds.push_back(card->id);
        }

        ImGui::PushID(static_cast<int>(index));
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 center(
            start.x + circleRadius + 1.0f,
            start.y + circleRadius);

        ImGui::Indent(textIndent);
        if (ImGui::Button(card->name.c_str()))
        {
            navigation_.Request(card->id);
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, GuideTheme::MutedTextColor);
        ImGui::TextUnformatted(step.functionName.c_str());
        ImGui::PopStyleColor();
        ImGui::TextWrapped("%s", step.description.c_str());
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::Unindent(textIndent);

        if (index + 1 < feature.steps.size())
        {
            drawList->AddLine(
                ImVec2(center.x, center.y + circleRadius),
                ImVec2(center.x, ImGui::GetCursorScreenPos().y),
                GuideTheme::LineColor,
                2.0f);
        }
        drawList->AddCircleFilled(
            center,
            circleRadius,
            GuideTheme::GetPlateColors(card->groupColor).lit);
        drawList->AddCircle(center, circleRadius, GuideTheme::LineColor, 0, 2.0f);

        const std::string number = std::to_string(index + 1);
        const ImVec2 numberSize = GuideTheme::GetTextSize(GuideTheme::BodyFontSize, number.c_str());
        drawList->AddText(
            ImGui::GetFont(),
            GuideTheme::BodyFontSize,
            ImVec2(
                center.x - numberSize.x * 0.5f,
                center.y - numberSize.y * 0.5f),
            GuideTheme::TextColor,
            number.c_str());
        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    GuideTheme::DrawHeading("거쳐 가는 카드", GuideTheme::HeadingFontSize);
    DrawCardGrid(cardIds);
}

void EngineGuideWindow::DrawFeatureButtons(
    const std::vector<const GuideFeature*>& features)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float rightEdge =
        ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

    for (std::size_t index = 0; index < features.size(); ++index)
    {
        const std::string label = "?  " + features[index]->name;
        const float width =
            ImGui::CalcTextSize(label.c_str()).x +
            style.FramePadding.x * 2.0f;

        // 앞 Button 옆에 들어갈 자리가 있을 때만 같은 줄에 이어 그립니다.
        if (index > 0 &&
            ImGui::GetItemRectMax().x + style.ItemSpacing.x + width <= rightEdge)
        {
            ImGui::SameLine();
        }
        if (ImGui::Button(label.c_str()))
        {
            navigation_.Request(features[index]->id);
        }
    }
}

void EngineGuideWindow::DrawCardGrid(const std::vector<std::string>& cardIds)
{
    constexpr float minimumCardWidth = 270.0f;
    constexpr float cardSpacing = 12.0f;

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const int columnCount = (std::max)(
        1,
        static_cast<int>(
            (availableWidth + cardSpacing) / (minimumCardWidth + cardSpacing)));
    const float cardWidth = std::floor(
        (availableWidth - cardSpacing * static_cast<float>(columnCount - 1)) /
        static_cast<float>(columnCount));

    // 같은 묶음의 카드 높이를 맞추기 위해 가장 긴 설명을 기준으로 삼습니다.
    float roleHeight = 0.0f;
    for (const std::string& cardId : cardIds)
    {
        if (const GuideCard* const card = guide_.FindCard(cardId))
        {
            roleHeight = (std::max)(
                roleHeight,
                GuideTheme::GetTextSize(
                    GuideTheme::SmallFontSize,
                    card->role.c_str(),
                    cardWidth - 28.0f).y);
        }
    }
    const float cardHeight =
        GuidePictureRenderer::GetHeight(cardWidth) +
        GuideTheme::BodyFontSize + roleHeight + 30.0f;

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(cardSpacing, cardSpacing));
    int column = 0;
    for (const std::string& cardId : cardIds)
    {
        const GuideCard* const card = guide_.FindCard(cardId);
        if (card == nullptr)
        {
            continue;
        }

        if (column > 0)
        {
            ImGui::SameLine();
        }
        DrawCardButton(*card, cardWidth, cardHeight);
        column = (column + 1) % columnCount;
    }
    ImGui::PopStyleVar();
}

void EngineGuideWindow::DrawCardButton(
    const GuideCard& card,
    float width,
    float height)
{
    const ImVec2 minimum = ImGui::GetCursorScreenPos();
    const ImVec2 maximum(minimum.x + width, minimum.y + height);

    ImGui::PushID(card.id.c_str());
    if (ImGui::InvisibleButton("Card", ImVec2(width, height)))
    {
        navigation_.Request(card.id);
    }
    const bool isHovered = ImGui::IsItemHovered();
    ImGui::PopID();
    if (isHovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(minimum, maximum, GuideTheme::PaperColor);
    GuidePictureRenderer::Draw(card, { minimum.x, minimum.y }, width);

    const float pictureBottom =
        minimum.y + GuidePictureRenderer::GetHeight(width);
    drawList->AddLine(
        ImVec2(minimum.x, pictureBottom),
        ImVec2(maximum.x, pictureBottom),
        GuideTheme::LineColor,
        2.0f);
    drawList->AddText(
        ImGui::GetFont(),
        GuideTheme::BodyFontSize,
        ImVec2(minimum.x + 14.0f, pictureBottom + 9.0f),
        GuideTheme::TextColor,
        card.name.c_str());
    drawList->AddText(
        ImGui::GetFont(),
        GuideTheme::SmallFontSize,
        ImVec2(minimum.x + 14.0f, pictureBottom + GuideTheme::BodyFontSize + 15.0f),
        GuideTheme::MutedTextColor,
        card.role.c_str(),
        nullptr,
        width - 28.0f);
    drawList->AddRect(
        minimum,
        maximum,
        isHovered ? GuideTheme::FlameColor : GuideTheme::LineColor,
        3.0f,
        2.0f);
}
