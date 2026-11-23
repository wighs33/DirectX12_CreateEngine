#include "EngineGuide.h"

#include "json.hpp"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr int GuideFormatVersion = 1;

    using Json = nlohmann::json;

    bool ReportLoadError(
        const std::filesystem::path& filePath,
        const std::string& reason)
    {
        std::fprintf(
            stderr,
            "엔진 도감을 읽지 못했습니다: %s (%s)\n",
            filePath.string().c_str(),
            reason.c_str());
        return false;
    }

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

    template <typename T>
    const T* FindById(const std::vector<T>& items, const std::string& id)
    {
        const auto found = std::find_if(
            items.begin(),
            items.end(),
            [&id](const T& item) { return item.id == id; });
        return found == items.end() ? nullptr : &*found;
    }
}

bool EngineGuide::Load(const std::filesystem::path& filePath)
{
    try
    {
        std::ifstream input(filePath);
        if (!input.is_open())
        {
            return ReportLoadError(filePath, "파일을 열 수 없습니다");
        }

        const Json guideJson = Json::parse(input);
        if (guideJson.at("version").get<int>() != GuideFormatVersion)
        {
            return ReportLoadError(filePath, "지원하지 않는 version입니다");
        }

        EngineGuide loadedGuide;
        for (const Json& cardJson : guideJson.at("cards"))
        {
            loadedGuide.cards_.push_back(ReadCard(cardJson));
        }
        for (const Json& featureJson : guideJson.at("features"))
        {
            loadedGuide.features_.push_back(ReadFeature(featureJson));
        }
        for (const Json& groupJson : guideJson.at("groups"))
        {
            GuideGroup group;
            group.name = groupJson.at("name").get<std::string>();
            group.color = groupJson.at("color").get<std::string>();
            group.cardIds =
                groupJson.at("cards").get<std::vector<std::string>>();

            // 카드 화면에서 자기 묶음의 이름과 색을 바로 쓸 수 있도록 복사해 둡니다.
            for (GuideCard& card : loadedGuide.cards_)
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
            loadedGuide.groups_.push_back(std::move(group));
        }

        if (!loadedGuide.ValidateReferences())
        {
            return ReportLoadError(filePath, "없는 카드를 가리키는 이름이 있습니다");
        }

        *this = std::move(loadedGuide);
        return true;
    }
    catch (const std::exception& exception)
    {
        return ReportLoadError(filePath, exception.what());
    }
}

const std::vector<GuideGroup>& EngineGuide::GetGroups() const
{
    return groups_;
}

const std::vector<GuideCard>& EngineGuide::GetCards() const
{
    return cards_;
}

const std::vector<GuideFeature>& EngineGuide::GetFeatures() const
{
    return features_;
}

const GuideCard* EngineGuide::FindCard(const std::string& id) const
{
    return FindById(cards_, id);
}

const GuideFeature* EngineGuide::FindFeature(const std::string& id) const
{
    return FindById(features_, id);
}

bool EngineGuide::ValidateReferences() const
{
    bool isValid = true;
    const auto checkCard = [this, &isValid](
        const std::string& owner,
        const std::string& cardId)
    {
        if (FindCard(cardId) == nullptr)
        {
            std::fprintf(
                stderr,
                "엔진 도감: %s이(가) 없는 카드 '%s'을(를) 가리킵니다.\n",
                owner.c_str(),
                cardId.c_str());
            isValid = false;
        }
    };

    for (const GuideGroup& group : groups_)
    {
        for (const std::string& cardId : group.cardIds)
            checkCard(group.name, cardId);
    }
    for (const GuideCard& card : cards_)
    {
        for (const std::string& cardId : card.links)
            checkCard(card.id, cardId);
    }
    for (const GuideFeature& feature : features_)
    {
        for (const GuideStep& step : feature.steps)
            checkCard(feature.id, step.cardId);
    }

    return isValid;
}
