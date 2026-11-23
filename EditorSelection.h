#pragma once

#include <cstddef>
#include <limits>

class GameObject;
class Scene;

// 지금 고른 물체와 삭제 요청을 기억합니다.
// 포인터 대신 번호를 보관하므로 물체 목록이 다시 배치되어도 안전합니다.
class EditorSelection final
{
public:
    void Select(std::size_t objectIndex);
    void Clear();

    bool IsSelected(std::size_t objectIndex) const;

    // 고른 물체가 없거나 이미 사라졌으면 nullptr을 반환합니다.
    GameObject* GetSelected(Scene& scene) const;

    // 목록을 도는 도중에 물체가 사라지지 않도록 삭제는 요청만 해 둡니다.
    void RequestDelete();
    void ProcessDeleteRequest(Scene& scene);

private:
    static constexpr std::size_t InvalidIndex =
        (std::numeric_limits<std::size_t>::max)();

    std::size_t selectedIndex_ = InvalidIndex;
    bool isDeleteRequested_ = false;
};
