#include "RenderStatistics.h"

#include <cstdio>
#include <cwchar>

void RenderStatistics::BeginFrame(std::size_t sceneObjectCount)
{
    sceneObjectCount_ = static_cast<std::uint32_t>(sceneObjectCount);
    renderedObjectCount_ = 0;
    culledObjectCount_ = 0;
    drawCallCount_ = 0;
    triangleCount_ = 0;
}

void RenderStatistics::SetVisibility(
    std::size_t renderedCount,
    std::size_t culledCount)
{
    renderedObjectCount_ = static_cast<std::uint32_t>(renderedCount);
    culledObjectCount_ = static_cast<std::uint32_t>(culledCount);
}

void RenderStatistics::AddDraw(std::uint32_t triangleCount)
{
    ++drawCallCount_;
    triangleCount_ += triangleCount;
}

bool RenderStatistics::Update(float deltaTime)
{
    constexpr float updateInterval = 1.0f;

    elapsedTime_ += deltaTime;
    ++frameCount_;
    if (elapsedTime_ < updateInterval)
    {
        return false;
    }

    fps_ = static_cast<float>(frameCount_) / elapsedTime_;
    frameTimeMs_ = elapsedTime_ * 1000.0f / static_cast<float>(frameCount_);
    elapsedTime_ = 0.0f;
    frameCount_ = 0;
    return true;
}

std::wstring RenderStatistics::CreateTitle() const
{
    wchar_t title[256]{};
    swprintf_s(
        title,
        L"DirectX 12 Study | FPS: %.1f | Frame: %.2f ms | Objects: %u | Rendered: %u | Culled: %u | DrawCalls: %u | Triangles: %u",
        fps_,
        frameTimeMs_,
        sceneObjectCount_,
        renderedObjectCount_,
        culledObjectCount_,
        drawCallCount_,
        triangleCount_);
    return title;
}
