#include "EngineGuideReader.h"

#include "json.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr int GuideFormatVersion = 2;

    using Json = nlohmann::json;

    GuidePoint ReadPoint(const Json& pointJson)
    {
        return
        {
            pointJson.at(0).get<float>(),
            pointJson.at(1).get<float>()
        };
    }

    GuideFill ReadFill(const std::string& fillName)
    {
        if (fillName == "none")
            return GuideFill::None;
        if (fillName == "box")
            return GuideFill::Box;
        if (fillName == "highlight")
            return GuideFill::Highlight;
        if (fillName == "dot")
            return GuideFill::Dot;

        throw std::runtime_error("알 수 없는 fill: " + fillName);
    }

    GuideShape ReadShape(const Json& shapeJson)
    {
        GuideShape shape;
        shape.isDashed = shapeJson.value("dashed", false);

        const std::string typeName = shapeJson.at("shape").get<std::string>();
        if (typeName == "line")
        {
            shape.type = GuideShapeType::Line;
            shape.isClosed = shapeJson.value("closed", false);

            const std::string arrow = shapeJson.value("arrow", "none");
            shape.hasStartArrow = arrow == "both";
            shape.hasEndArrow = arrow == "both" || arrow == "end";

            for (const Json& pointJson : shapeJson.at("points"))
            {
                shape.points.push_back(ReadPoint(pointJson));
            }
            return shape;
        }

        shape.position =
        {
            shapeJson.at("x").get<float>(),
            shapeJson.at("y").get<float>()
        };

        if (typeName == "rect")
        {
            shape.type = GuideShapeType::Rect;
            shape.fill = ReadFill(shapeJson.value("fill", "box"));
            shape.rotationDegrees = shapeJson.value("rotation", 0.0f);
            shape.size =
            {
                shapeJson.at("w").get<float>(),
                shapeJson.at("h").get<float>()
            };
        }
        else if (typeName == "circle")
        {
            shape.type = GuideShapeType::Circle;
            shape.fill = ReadFill(shapeJson.value("fill", "dot"));
            shape.radius = shapeJson.at("r").get<float>();
        }
        else if (typeName == "text")
        {
            shape.type = GuideShapeType::Text;
            shape.textSize = shapeJson.value("size", 14.0f);
            shape.isLeftAligned = shapeJson.value("align", "center") == "left";
            shape.text = shapeJson.at("text").get<std::string>();
        }
        else
        {
            throw std::runtime_error("알 수 없는 shape: " + typeName);
        }

        return shape;
    }

    GuideCard ReadCard(const Json& cardJson)
    {
        GuideCard card;
        card.id = cardJson.at("id").get<std::string>();
        card.name = cardJson.at("name").get<std::string>();
        card.file = cardJson.at("file").get<std::string>();
        card.role = cardJson.at("role").get<std::string>();
        card.without = cardJson.at("without").get<std::string>();
        card.limit = cardJson.at("limit").get<std::string>();
        card.unreal = cardJson.at("unreal").get<std::string>();
        card.links = cardJson.at("links").get<std::vector<std::string>>();

        for (const Json& functionJson : cardJson.at("functions"))
        {
            card.functions.push_back(
                {
                    functionJson.at("name").get<std::string>(),
                    functionJson.at("description").get<std::string>()
                });
        }
        for (const Json& shapeJson : cardJson.at("picture"))
        {
            card.picture.push_back(ReadShape(shapeJson));
        }

        return card;
    }

    GuideFeature ReadFeature(const Json& featureJson)
    {
        GuideFeature feature;
        feature.id = featureJson.at("id").get<std::string>();
        feature.name = featureJson.at("name").get<std::string>();
        feature.howTo = featureJson.at("howTo").get<std::string>();

        for (const Json& stepJson : featureJson.at("steps"))
        {
            feature.steps.push_back(
                {
                    stepJson.at("card").get<std::string>(),
                    stepJson.at("function").get<std::string>(),
                    stepJson.at("description").get<std::string>()
                });
        }

        return feature;
    }

    GuideStage ReadStage(const Json& stageJson)
    {
        GuideStage stage;
        stage.id = stageJson.at("id").get<std::string>();
        stage.name = stageJson.at("name").get<std::string>();
        stage.date = stageJson.at("date").get<std::string>();
        stage.made = stageJson.at("made").get<std::string>();
        stage.learned = stageJson.at("learned").get<std::string>();
        stage.checked = stageJson.at("checked").get<std::string>();

        for (const Json& commitJson : stageJson.at("commits"))
        {
            stage.commits.push_back(
                {
                    commitJson.at("hash").get<std::string>(),
                    commitJson.at("title").get<std::string>(),
                    commitJson.at("cards").get<std::vector<std::string>>()
                });
        }

        return stage;
    }

    GuideGroup ReadGroup(const Json& groupJson)
    {
        GuideGroup group;
        group.name = groupJson.at("name").get<std::string>();
        group.color = groupJson.at("color").get<std::string>();
        group.cardIds = groupJson.at("cards").get<std::vector<std::string>>();
        return group;
    }

    // 카드 화면에서 자기 묶음의 이름과 색을 바로 쓸 수 있도록 복사해 둡니다.
    void CopyGroupToCards(
        const GuideGroup& group,
        std::vector<GuideCard>& cards)
    {
        for (GuideCard& card : cards)
        {
            if (std::find(
                    group.cardIds.begin(),
                    group.cardIds.end(),
                    card.id) != group.cardIds.end())
            {
                card.groupName = group.name;
                card.groupColor = group.color;
            }
        }
    }
}

EngineGuideData EngineGuideReader::Read(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        throw std::runtime_error("파일을 열 수 없습니다");
    }

    const Json guideJson = Json::parse(input);
    if (guideJson.at("version").get<int>() != GuideFormatVersion)
    {
        throw std::runtime_error("지원하지 않는 version입니다");
    }

    EngineGuideData data;
    for (const Json& cardJson : guideJson.at("cards"))
    {
        data.cards.push_back(ReadCard(cardJson));
    }
    for (const Json& featureJson : guideJson.at("features"))
    {
        data.features.push_back(ReadFeature(featureJson));
    }
    for (const Json& stageJson : guideJson.at("stages"))
    {
        data.stages.push_back(ReadStage(stageJson));
    }
    for (const Json& groupJson : guideJson.at("groups"))
    {
        data.groups.push_back(ReadGroup(groupJson));
        CopyGroupToCards(data.groups.back(), data.cards);
    }
    return data;
}
