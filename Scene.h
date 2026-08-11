#pragma once

#include "GameObject.h"

#include <cstddef>
#include <vector>

// 여러 GameObject의 수명과 목록만 관리하는 최소한의 CPU 측 Scene Container입니다.
class Scene final
{
public:
    std::size_t AddGameObject(GameObject gameObject);

    std::vector<GameObject>& GetGameObjects();
    const std::vector<GameObject>& GetGameObjects() const;

    std::size_t GetGameObjectCount() const;

private:
    std::vector<GameObject> gameObjects_;
};
