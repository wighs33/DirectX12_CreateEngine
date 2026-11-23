#pragma once

#include <filesystem>
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

// 엔진 도감의 카드와 기능 설명을 JSON File에서 읽어 이름으로 찾아 줍니다.
class EngineGuide final
{
public:
    // 모든 내용을 읽고 검사에 성공한 경우에만 기존 내용을 교체합니다.
    bool Load(const std::filesystem::path& filePath);

    const std::vector<GuideGroup>& GetGroups() const;
    const std::vector<GuideCard>& GetCards() const;
    const std::vector<GuideFeature>& GetFeatures() const;

    // 찾는 이름이 없으면 nullptr을 반환합니다.
    const GuideCard* FindCard(const std::string& id) const;
    const GuideFeature* FindFeature(const std::string& id) const;

private:
    // 카드끼리의 연결과 기능의 순서가 실제 있는 카드를 가리키는지 확인합니다.
    bool ValidateReferences() const;

    std::vector<GuideGroup> groups_;
    std::vector<GuideCard> cards_;
    std::vector<GuideFeature> features_;
};
