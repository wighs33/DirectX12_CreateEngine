#include "Scene.h"

#include <algorithm>
#include <limits>
#include <utility>

std::size_t Scene::AddGameObject(GameObject gameObject)
{
    const std::size_t objectIndex = gameObjects_.size();
    gameObjects_.push_back(std::move(gameObject));
    return objectIndex;
}

bool Scene::RemoveGameObject(std::size_t objectIndex)
{
    if (objectIndex >= gameObjects_.size())
    {
        return false;
    }

    gameObjects_.erase(gameObjects_.begin() + objectIndex);
    return true;
}

std::vector<GameObject>& Scene::GetGameObjects()
{
    return gameObjects_;
}

const std::vector<GameObject>& Scene::GetGameObjects() const
{
    return gameObjects_;
}

std::size_t Scene::GetGameObjectCount() const
{
    return gameObjects_.size();
}

std::string Scene::CreateUniqueName(const std::string& baseName) const
{
    const auto isNameUsed = [this](const std::string& name)
    {
        return std::any_of(
            gameObjects_.begin(),
            gameObjects_.end(),
            [&name](const GameObject& gameObject)
            {
                return gameObject.GetName() == name;
            });
    };

    if (!isNameUsed(baseName))
    {
        return baseName;
    }

    for (std::size_t suffix = 1;
         suffix < (std::numeric_limits<std::size_t>::max)();
         ++suffix)
    {
        const std::string candidate =
            baseName + "_" + std::to_string(suffix);
        if (!isNameUsed(candidate))
        {
            return candidate;
        }
    }

    return baseName + "_New";
}

const SceneLighting& Scene::GetLighting() const
{
    return lighting_;
}
