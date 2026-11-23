#pragma once

#include "GameObject.h"
#include "SceneLighting.h"

#include <cstddef>
#include <string>
#include <vector>

// 여러 GameObject의 수명과 목록, 그리고 장면을 비추는 빛을 관리합니다.
class Scene final
{
public:
    std::size_t AddGameObject(GameObject gameObject);
    bool RemoveGameObject(std::size_t objectIndex);

    std::vector<GameObject>& GetGameObjects();
    const std::vector<GameObject>& GetGameObjects() const;

    std::size_t GetGameObjectCount() const;

    // 이미 있는 물체와 겹치지 않는 이름을 만듭니다.
    std::string CreateUniqueName(const std::string& baseName) const;

    const SceneLighting& GetLighting() const;

private:
    std::vector<GameObject> gameObjects_;
    SceneLighting lighting_;
};
