#include "EngineGuide.h"

#include "EngineGuideReader.h"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <utility>

namespace
{
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
        EngineGuide loadedGuide;
        loadedGuide.data_ = EngineGuideReader::Read(filePath);
        if (!loadedGuide.ValidateReferences())
        {
            throw std::runtime_error("없는 카드를 가리키는 이름이 있습니다");
        }

        *this = std::move(loadedGuide);
        return true;
    }
    catch (const std::exception& exception)
    {
        std::fprintf(
            stderr,
            "엔진 도감을 읽지 못했습니다: %s (%s)\n",
            filePath.string().c_str(),
            exception.what());
        return false;
    }
}

const std::vector<GuideGroup>& EngineGuide::GetGroups() const
{
    return data_.groups;
}

const std::vector<GuideCard>& EngineGuide::GetCards() const
{
    return data_.cards;
}

const std::vector<GuideFeature>& EngineGuide::GetFeatures() const
{
    return data_.features;
}

const GuideCard* EngineGuide::FindCard(const std::string& id) const
{
    return FindById(data_.cards, id);
}

const GuideFeature* EngineGuide::FindFeature(const std::string& id) const
{
    return FindById(data_.features, id);
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

    for (const GuideGroup& group : data_.groups)
    {
        for (const std::string& cardId : group.cardIds)
            checkCard(group.name, cardId);
    }
    for (const GuideCard& card : data_.cards)
    {
        for (const std::string& cardId : card.links)
            checkCard(card.id, cardId);
    }
    for (const GuideFeature& feature : data_.features)
    {
        for (const GuideStep& step : feature.steps)
            checkCard(feature.id, step.cardId);
    }

    return isValid;
}
