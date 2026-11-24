#pragma once

#include <string>
#include <vector>

// 카드 그림 안의 좌표입니다. 그림은 340 x 180 크기를 기준으로 배치합니다.
struct GuidePoint
{
    float x = 0.0f;
    float y = 0.0f;
};

enum class GuideShapeType
{
    Rect,
    Line,
    Circle,
    Text
};

enum class GuideFill
{
    None,
    Box,
    Highlight,
    Dot
};

// 카드 그림을 이루는 도형 하나이며 종류에 따라 필요한 값만 사용합니다.
struct GuideShape
{
    GuideShapeType type = GuideShapeType::Rect;
    GuideFill fill = GuideFill::Box;
    // Rect는 왼쪽 위, Circle은 중심, Text는 글자의 세로 중심입니다.
    GuidePoint position{};
    GuidePoint size{};
    float radius = 0.0f;
    float rotationDegrees = 0.0f;
    float textSize = 14.0f;
    bool isDashed = false;
    bool isClosed = false;
    bool hasStartArrow = false;
    bool hasEndArrow = false;
    bool isLeftAligned = false;
    std::vector<GuidePoint> points;
    std::string text;
};

struct GuideFunction
{
    std::string name;
    std::string description;
};

// Class 하나를 설명하는 카드입니다.
struct GuideCard
{
    std::string id;
    std::string name;
    std::string file;
    std::string role;
    std::string without;
    std::string limit;
    std::string unreal;
    std::string groupName;
    std::string groupColor;
    std::vector<GuideFunction> functions;
    std::vector<std::string> links;
    std::vector<GuideShape> picture;
};

struct GuideStep
{
    std::string cardId;
    std::string functionName;
    std::string description;
};

// Editor 기능 하나가 어떤 카드를 거쳐 동작하는지 설명합니다.
struct GuideFeature
{
    std::string id;
    std::string name;
    std::string howTo;
    std::vector<GuideStep> steps;
};

struct GuideGroup
{
    std::string name;
    std::string color;
    std::vector<std::string> cardIds;
};

// 도감 파일에 적힌 내용 전체입니다.
struct EngineGuideData
{
    std::vector<GuideGroup> groups;
    std::vector<GuideCard> cards;
    std::vector<GuideFeature> features;
};
