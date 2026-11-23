#include "GuidePictureRenderer.h"

#include "GuideTheme.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace
{
    // 카드 그림의 좌표는 이 크기를 기준으로 적혀 있습니다.
    constexpr float PictureWidth = 340.0f;
    constexpr float PictureHeight = 180.0f;
    constexpr float LineHeightPerTextSize = 1.33f;

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
                GuideTheme::LineColor,
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
                GuideTheme::LineColor,
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
            GuideTheme::LineColor);
    }

    ImU32 GetFillColor(GuideFill fill)
    {
        switch (fill)
        {
        case GuideFill::Highlight:
            return GuideTheme::HighlightColor;
        case GuideFill::Dot:
            return GuideTheme::FlameColor;
        case GuideFill::Box:
        case GuideFill::None:
            break;
        }
        return GuideTheme::BoxColor;
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
                GuideTheme::LineColor,
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
        const ImVec2 textSize = GuideTheme::GetTextSize(fontSize, shape.text.c_str());
        const float left = origin.x + shape.position.x * scale -
            (shape.isLeftAligned ? 0.0f : textSize.x * 0.5f);
        const float top =
            origin.y + shape.position.y * scale - textSize.y * 0.5f;

        drawList->AddText(
            ImGui::GetFont(),
            fontSize,
            ImVec2(left, top),
            GuideTheme::TextColor,
            shape.text.c_str());
    }
}

void GuidePictureRenderer::Draw(
    const GuideCard& card,
    const GuidePoint& position,
    float width)
{
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float scale = width / PictureWidth;
    const float thickness = (std::max)(1.0f, 1.8f * scale);
    const ImVec2 origin(position.x, position.y);
    const ImVec2 end(origin.x + width, origin.y + GetHeight(width));
    const GuideTheme::PlateColors plate = GuideTheme::GetPlateColors(card.groupColor);

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
            drawList->AddCircle(center, radius, GuideTheme::LineColor, 0, thickness);
            break;
        }
        case GuideShapeType::Text:
            DrawTextShape(drawList, shape, origin, scale);
            break;
        }
    }
    drawList->PopClipRect();
}

float GuidePictureRenderer::GetHeight(float width)
{
    return width * PictureHeight / PictureWidth;
}
