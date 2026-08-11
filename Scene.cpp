#include "Scene.h"

#include <utility>

void Scene::AddGameObject(GameObject gameObject)
{
    gameObjects_.push_back(std::move(gameObject));
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
