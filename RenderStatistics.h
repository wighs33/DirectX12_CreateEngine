#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// 한 Frame 동안 그린 양을 세고 1초마다 평균 FPS를 구합니다.
class RenderStatistics final
{
public:
    // 이번 Frame에 그릴 물체 수를 적고 나머지 개수를 0으로 되돌립니다.
    void BeginFrame(std::size_t sceneObjectCount);

    // 카메라에 보여 그린 물체와 시야 밖이라 뺀 물체의 수입니다.
    void SetVisibility(std::size_t renderedCount, std::size_t culledCount);
    void AddDraw(std::uint32_t triangleCount);

    // 1초가 지나 FPS를 새로 구한 Frame에만 true를 반환합니다.
    bool Update(float deltaTime);

    std::wstring CreateTitle() const;

private:
    float fps_ = 0.0f;
    float frameTimeMs_ = 0.0f;
    std::uint32_t sceneObjectCount_ = 0;
    std::uint32_t renderedObjectCount_ = 0;
    std::uint32_t culledObjectCount_ = 0;
    std::uint32_t drawCallCount_ = 0;
    std::uint32_t triangleCount_ = 0;

    float elapsedTime_ = 0.0f;
    std::uint32_t frameCount_ = 0;
};
