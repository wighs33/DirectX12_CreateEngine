#include "Timer.h"

#include <Windows.h>

Timer::Timer()
{
    LARGE_INTEGER frequency{};
    if (QueryPerformanceFrequency(&frequency))
    {
        frequency_ = frequency.QuadPart;
    }

    LARGE_INTEGER currentCounter{};
    QueryPerformanceCounter(&currentCounter);
    previousCounter_ = currentCounter.QuadPart;
}

void Timer::Tick()
{
    LARGE_INTEGER currentCounter{};
    QueryPerformanceCounter(&currentCounter);

    const std::int64_t elapsedCounter = currentCounter.QuadPart - previousCounter_;
    previousCounter_ = currentCounter.QuadPart;

    const double elapsedSeconds =
        static_cast<double>(elapsedCounter) / static_cast<double>(frequency_);
    deltaTime_ = elapsedSeconds > 0.0
        ? static_cast<float>(elapsedSeconds)
        : 0.0f;
}

float Timer::GetDeltaTime() const
{
    return deltaTime_;
}
