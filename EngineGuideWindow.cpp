#include "EngineGuideWindow.h"

#include "imgui.h"

#include <Windows.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

namespace
{
    constexpr float PictureWidth = 340.0f;
    constexpr float PictureHeight = 180.0f;

    // ImGui의 글꼴 크기는 글자의 줄 높이이므로 일반적인 글자 크기보다 크게 지정합니다.
    constexpr float BodyFontSize = 20.0f;
    constexpr float SmallFontSize = 18.0f;
    constexpr float HeadingFontSize = 26.0f;
    constexpr float TitleFontSize = 36.0f;
    constexpr float LineHeightPerTextSize = 1.33f;

    constexpr ImU32 BackgroundColor = IM_COL32(22, 18, 31, 255);
    constexpr ImU32 PaperColor = IM_COL32(34, 28, 48, 255);
    constexpr ImU32 TextColor = IM_COL32(240, 236, 250, 255);
    constexpr ImU32 MutedTextColor = IM_COL32(176, 166, 200, 255);
    constexpr ImU32 LineColor = IM_COL32(205, 195, 234, 255);
    constexpr ImU32 HairlineColor = IM_COL32(64, 55, 90, 255);
    constexpr ImU32 FlameColor = IM_COL32(255, 138, 76, 255);
    constexpr ImU32 BoxColor = IM_COL32(44, 37, 64, 255);
    constexpr ImU32 HighlightColor = IM_COL32(106, 90, 31, 255);

    // 카드 그림판은 셀 셰이딩처럼 밝은 면과 그늘 두 단으로만 나눕니다.
    struct PlateColors
    {
        ImU32 lit;
        ImU32 shade;
    };

    PlateColors GetPlateColors(const std::string& groupColor)
    {
        if (groupColor == "flame")
            return { IM_COL32(90, 54, 35, 255), IM_COL32(67, 39, 25, 255) };
        if (groupColor == "violet")
            return { IM_COL32(61, 52, 112, 255), IM_COL32(45, 38, 86, 255) };
        if (groupColor == "teal")
            return { IM_COL32(31, 77, 71, 255), IM_COL32(22, 58, 54, 255) };
        if (groupColor == "rose")
            return { IM_COL32(92, 45, 70, 255), IM_COL32(69, 32, 51, 255) };

        return { IM_COL32(58, 64, 88, 255), IM_COL32(43, 48, 68, 255) };
    }

    ImVec2 GetTextSize(float fontSize, const char* text, float wrapWidth = 0.0f)
    {
        return ImGui::GetFont()->CalcTextSizeA(
            fontSize,
            FLT_MAX,
            wrapWidth,
            text);
    }

    void DrawHeading(const char* text, float fontSize)
    {
        ImGui::PushFont(nullptr, fontSize);
        ImGui::TextWrapped("%s", text);
        ImGui::PopFont();
    }

    void DrawMutedText(const std::string& text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, MutedTextColor);
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::PopStyleColor();
    }

    void DrawSection(const char* heading, const std::string& text)
    {
        DrawHeading(heading, HeadingFontSize);
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::Spacing();
    }

    void DrawDashedSegment(
        ImDrawList* drawList,
        const ImVec2& start,
        const ImVec2& end,
        float thickness,
        float scale)
    {
        const float deltaX = end.x - start.x;
        const float deltaY = end.y - start.y;
        const float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        const float dashLength = 5.0f * scale;
        const float dashStep = 9.0f * scale;
        if (length <= 0.0f || dashStep <= 0.0f)
        {
            return;
        }

        for (float offset = 0.0f; offset < length; offset += dashStep)
        {
            const float dashEnd = (std::min)(offset + dashLength, length);
            drawList->AddLine(
                ImVec2(
                    start.x + deltaX * offset / length,
                    start.y + deltaY * offset / length),
                ImVec2(
                    start.x + deltaX * dashEnd / length,
                    start.y + deltaY * dashEnd / length),
                LineColor,
                thickness);
        }
    }

    void DrawStroke(
        ImDrawList* drawList,
        const std::vector<ImVec2>& points,
        bool isClosed,
        bool isDashed,
        float thickness,
        float scale)
    {
        if (points.size() < 2)
        {
            return;
        }

        if (!isDashed)
        {
            drawList->AddPolyline(
                points.data(),
                static_cast<int>(points.size()),
                LineColor,
                thickness,
                isClosed ? ImDrawFlags_Closed : ImDrawFlags_None);
            return;
        }

        for (std::size_t index = 1; index < points.size(); ++index)
        {
            DrawDashedSegment(
                drawList,
                points[index - 1],
                points[index],
                thickness,
                scale);
        }
        if (isClosed)
        {
            DrawDashedSegment(
                drawList,
                points.back(),
                points.front(),
                thickness,
                scale);
        }
    }

    // tip에서 끝나는 화살촉을 그립니다. from은 선이 들어오는 방향을 정합니다.
    void DrawArrowHead(
        ImDrawList* drawList,
        const ImVec2& from,
        const ImVec2& tip,
        float scale)
    {
        const float deltaX = tip.x - from.x;
        const float deltaY = tip.y - from.y;
        const float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (length <= 0.0f)
        {
            return;
        }

        const float directionX = deltaX / length;
        const float directionY = deltaY / length;
        const float headLength = 11.0f * scale;
        const float headHalfWidth = 5.5f * scale;
        const ImVec2 base(
            tip.x - directionX * headLength,
            tip.y - directionY * headLength);
        drawList->AddTriangleFilled(
            tip,
            ImVec2(
                base.x - directionY * headHalfWidth,
                base.y + directionX * headHalfWidth),
            ImVec2(
                base.x + directionY * headHalfWidth,
                base.y - directionX * headHalfWidth),
            LineColor);
    }

    ImU32 GetFillColor(GuideFill fill)
    {
        switch (fill)
        {
        case GuideFill::Highlight:
            return HighlightColor;
        case GuideFill::Dot:
            return FlameColor;
        case GuideFill::Box:
        case GuideFill::None:
            break;
        }
        return BoxColor;
    }

    void DrawRectShape(
        ImDrawList* drawList,
        const GuideShape& shape,
        const ImVec2& origin,
        float scale,
        float thickness)
    {
        const ImVec2 minimum(
            origin.x + shape.position.x * scale,
            origin.y + shape.position.y * scale);
        const ImVec2 maximum(
            minimum.x + shape.size.x * scale,
            minimum.y + shape.size.y * scale);

        if (shape.rotationDegrees == 0.0f && !shape.isDashed)
        {
            if (shape.fill != GuideFill::None)
            {
                drawList->AddRectFilled(
                    minimum,
                    maximum,
                    GetFillColor(shape.fill),
                    4.0f * scale);
            }
            drawList->AddRect(
                minimum,
                maximum,
                LineColor,
                4.0f * scale,
                thickness);
            return;
        }

        const ImVec2 center(
            (minimum.x + maximum.x) * 0.5f,
            (minimum.y + maximum.y) * 0.5f);
        const float radians = shape.rotationDegrees * 3.14159265f / 180.0f;
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        std::vector<ImVec2> corners =
        {
            ImVec2(minimum.x, minimum.y),
            ImVec2(maximum.x, minimum.y),
            ImVec2(maximum.x, maximum.y),
            ImVec2(minimum.x, maximum.y)
        };
        for (ImVec2& corner : corners)
        {
            const float offsetX = corner.x - center.x;
            const float offsetY = corner.y - center.y;
            corner = ImVec2(
                center.x + offsetX * cosine - offsetY * sine,
                center.y + offsetX * sine + offsetY * cosine);
        }

        if (shape.fill != GuideFill::None)
        {
            drawList->AddConvexPolyFilled(
                corners.data(),
                static_cast<int>(corners.size()),
                GetFillColor(shape.fill));
        }
        DrawStroke(
            drawList,
            corners,
            true,
            shape.isDashed,
            thickness,
            scale);
    }

    void DrawLineShape(
        ImDrawList* drawList,
        const GuideShape& shape,
        const ImVec2& origin,
        float scale,
        float thickness)
    {
        std::vector<ImVec2> points;
        points.reserve(shape.points.size());
        for (const GuidePoint& point : shape.points)
        {
            points.push_back(ImVec2(
                origin.x + point.x * scale,
                origin.y + point.y * scale));
        }

        DrawStroke(
            drawList,
            points,
            shape.isClosed,
            shape.isDashed,
            thickness,
            scale);
        if (points.size() < 2)
        {
            return;
        }

        if (shape.hasEndArrow)
        {
            DrawArrowHead(
                drawList,
                points[points.size() - 2],
                points.back(),
                scale);
        }
        if (shape.hasStartArrow)
        {
            DrawArrowHead(drawList, points[1], points.front(), scale);
        }
    }

    void DrawTextShape(
        ImDrawList* drawList,
        const GuideShape& shape,
        const ImVec2& origin,
        float scale)
    {
        const float fontSize =
            std::round(shape.textSize * LineHeightPerTextSize * scale);
        const ImVec2 textSize = GetTextSize(fontSize, shape.text.c_str());
        const float left = origin.x + shape.position.x * scale -
            (shape.isLeftAligned ? 0.0f : textSize.x * 0.5f);
        const float top =
            origin.y + shape.position.y * scale - textSize.y * 0.5f;

        drawList->AddText(
            ImGui::GetFont(),
            fontSize,
            ImVec2(left, top),
            TextColor,
            shape.text.c_str());
    }
}

void EngineGuideWindow::Initialize(const std::filesystem::path& guideFilePath)
{
    LoadFont();
    isLoaded_ = guide_.Load(guideFilePath);
}

void EngineGuideWindow::LoadFont()
{
    ImFontAtlas* const fonts = ImGui::GetIO().Fonts;

    // 첫 번째 글꼴이 기존 Panel에 쓰이므로 기본 글꼴을 먼저 등록해 모양을 유지합니다.
    fonts->AddFontDefault();

    wchar_t windowsDirectory[MAX_PATH]{};
    if (GetWindowsDirectoryW(windowsDirectory, MAX_PATH) == 0)
    {
        return;
    }

    const std::filesystem::path fontPath =
        std::filesystem::path(windowsDirectory) / "Fonts" / "malgun.ttf";
    std::error_code fileError;
    if (!std::filesystem::exists(fontPath, fileError))
    {
        std::fprintf(
            stderr,
            "한글 글꼴을 찾지 못했습니다: %s\n",
            fontPath.string().c_str());
        return;
    }

    font_ = fonts->AddFontFromFileTTF(
        fontPath.u8string().c_str(),
        BodyFontSize);
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
    RequestNavigation(id);
}

void EngineGuideWindow::RequestNavigation(const std::string& id)
{
    requestedId_ = id;
    hasNavigationRequest_ = true;
}

void EngineGuideWindow::ApplyNavigation()
{
    if (isBackRequested_)
    {
        isBackRequested_ = false;
        if (!history_.empty())
        {
            currentId_ = history_.back();
            history_.pop_back();
            isScrollResetRequested_ = true;
        }
    }

    if (hasNavigationRequest_)
    {
        hasNavigationRequest_ = false;
        if (requestedId_ != currentId_)
        {
            history_.push_back(currentId_);
            currentId_ = requestedId_;
            isScrollResetRequested_ = true;
        }
    }
}

void EngineGuideWindow::Draw()
{
    if (!isOpen_)
    {
        return;
    }

    ApplyNavigation();

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

    ImGui::PushFont(font_, BodyFontSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, BackgroundColor);
    ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
    ImGui::PushStyleColor(ImGuiCol_Button, PaperColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HairlineColor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 14.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));

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

            const GuideCard* const card = guide_.FindCard(currentId_);
            const GuideFeature* const feature = guide_.FindFeature(currentId_);
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

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);
    ImGui::PopFont();
}

void EngineGuideWindow::DrawNavigationBar()
{
    ImGui::BeginDisabled(history_.empty());
    if (ImGui::Button("← 뒤로"))
    {
        isBackRequested_ = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(currentId_.empty());
    if (ImGui::Button("전체 카드"))
    {
        RequestNavigation(std::string());
    }
    ImGui::EndDisabled();

    const std::vector<GuideCard>& cards = guide_.GetCards();
    const auto current = std::find_if(
        cards.begin(),
        cards.end(),
        [this](const GuideCard& card) { return card.id == currentId_; });
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
            RequestNavigation(previous.id);
        }
    }
    if (current + 1 != cards.end())
    {
        const GuideCard& next = *(current + 1);
        ImGui::SameLine();
        if (ImGui::Button(("다음: " + next.name + "###Next").c_str()))
        {
            RequestNavigation(next.id);
        }
    }
}

void EngineGuideWindow::DrawHome()
{
    DrawMutedText(
        "클래스 하나가 카드 한 장입니다. 기능을 누르면 어떤 카드를 거쳐 움직이는지 볼 수 있습니다.");
    ImGui::Spacing();

    DrawHeading("기능", HeadingFontSize);
    std::vector<const GuideFeature*> features;
    for (const GuideFeature& feature : guide_.GetFeatures())
    {
        features.push_back(&feature);
    }
    DrawFeatureButtons(features);

    for (const GuideGroup& group : guide_.GetGroups())
    {
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        DrawHeading(group.name.c_str(), HeadingFontSize);
        DrawCardGrid(group.cardIds);
    }
}

void EngineGuideWindow::DrawCardPage(const GuideCard& card)
{
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float pictureWidth = (std::min)(440.0f, availableWidth * 0.5f);
    const ImVec2 picturePosition = ImGui::GetCursorScreenPos();
    DrawPicture(
        card,
        { picturePosition.x, picturePosition.y },
        pictureWidth);
    ImGui::Dummy(ImVec2(
        pictureWidth,
        pictureWidth * PictureHeight / PictureWidth));

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::BeginGroup();
    DrawHeading(card.name.c_str(), TitleFontSize);
    ImGui::TextWrapped("%s", card.role.c_str());
    DrawMutedText(card.groupName + " · " + card.file);
    ImGui::EndGroup();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    DrawSection("없으면 어떻게 될까", card.without);
    DrawSection("지금의 한계", card.limit);
    DrawSection("언리얼에서는", card.unreal);

    DrawHeading("하는 일", HeadingFontSize);
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
        DrawHeading("이 카드를 쓰는 기능", HeadingFontSize);
        DrawFeatureButtons(features);
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    DrawHeading("이어진 카드", HeadingFontSize);
    DrawCardGrid(card.links);
}

void EngineGuideWindow::DrawFeaturePage(const GuideFeature& feature)
{
    DrawHeading(feature.name.c_str(), TitleFontSize);
    ImGui::TextWrapped("%s", feature.howTo.c_str());
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    DrawHeading("움직이는 순서", HeadingFontSize);

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
            RequestNavigation(card->id);
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, MutedTextColor);
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
                LineColor,
                2.0f);
        }
        drawList->AddCircleFilled(
            center,
            circleRadius,
            GetPlateColors(card->groupColor).lit);
        drawList->AddCircle(center, circleRadius, LineColor, 0, 2.0f);

        const std::string number = std::to_string(index + 1);
        const ImVec2 numberSize = GetTextSize(BodyFontSize, number.c_str());
        drawList->AddText(
            ImGui::GetFont(),
            BodyFontSize,
            ImVec2(
                center.x - numberSize.x * 0.5f,
                center.y - numberSize.y * 0.5f),
            TextColor,
            number.c_str());
        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    DrawHeading("거쳐 가는 카드", HeadingFontSize);
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
            RequestNavigation(features[index]->id);
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
                GetTextSize(
                    SmallFontSize,
                    card->role.c_str(),
                    cardWidth - 28.0f).y);
        }
    }
    const float cardHeight =
        cardWidth * PictureHeight / PictureWidth +
        BodyFontSize + roleHeight + 30.0f;

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
        RequestNavigation(card.id);
    }
    const bool isHovered = ImGui::IsItemHovered();
    ImGui::PopID();
    if (isHovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(minimum, maximum, PaperColor);
    DrawPicture(card, { minimum.x, minimum.y }, width);

    const float pictureBottom =
        minimum.y + width * PictureHeight / PictureWidth;
    drawList->AddLine(
        ImVec2(minimum.x, pictureBottom),
        ImVec2(maximum.x, pictureBottom),
        LineColor,
        2.0f);
    drawList->AddText(
        ImGui::GetFont(),
        BodyFontSize,
        ImVec2(minimum.x + 14.0f, pictureBottom + 9.0f),
        TextColor,
        card.name.c_str());
    drawList->AddText(
        ImGui::GetFont(),
        SmallFontSize,
        ImVec2(minimum.x + 14.0f, pictureBottom + BodyFontSize + 15.0f),
        MutedTextColor,
        card.role.c_str(),
        nullptr,
        width - 28.0f);
    drawList->AddRect(
        minimum,
        maximum,
        isHovered ? FlameColor : LineColor,
        3.0f,
        2.0f);
}

void EngineGuideWindow::DrawPicture(
    const GuideCard& card,
    const GuidePoint& position,
    float width)
{
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float scale = width / PictureWidth;
    const float thickness = (std::max)(1.0f, 1.8f * scale);
    const ImVec2 origin(position.x, position.y);
    const ImVec2 end(origin.x + width, origin.y + PictureHeight * scale);
    const PlateColors plate = GetPlateColors(card.groupColor);

    drawList->PushClipRect(origin, end, true);
    drawList->AddRectFilled(origin, end, plate.lit);
    drawList->AddQuadFilled(
        ImVec2(origin.x + width * 0.77f, origin.y),
        ImVec2(end.x, origin.y),
        end,
        ImVec2(origin.x + width * 0.56f, end.y),
        plate.shade);

    for (const GuideShape& shape : card.picture)
    {
        switch (shape.type)
        {
        case GuideShapeType::Rect:
            DrawRectShape(drawList, shape, origin, scale, thickness);
            break;
        case GuideShapeType::Line:
            DrawLineShape(drawList, shape, origin, scale, thickness);
            break;
        case GuideShapeType::Circle:
        {
            const ImVec2 center(
                origin.x + shape.position.x * scale,
                origin.y + shape.position.y * scale);
            const float radius = shape.radius * scale;
            drawList->AddCircleFilled(
                center,
                radius,
                GetFillColor(shape.fill));
            drawList->AddCircle(center, radius, LineColor, 0, thickness);
            break;
        }
        case GuideShapeType::Text:
            DrawTextShape(drawList, shape, origin, scale);
            break;
        }
    }
    drawList->PopClipRect();
}
