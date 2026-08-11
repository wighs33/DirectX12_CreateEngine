#include "Scene.h"

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
