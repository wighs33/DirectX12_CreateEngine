#pragma once

#include <cstdint>

// Win32 고해상도 Counter를 이용해 이전 Frame과 현재 Frame 사이의 시간을 계산합니다.
class Timer final
{
public:
    Timer();

    void Tick();
    float GetDeltaTime() const;

private:
    std::int64_t frequency_ = 1;
    std::int64_t previousCounter_ = 0;
    float deltaTime_ = 0.0f;
};
