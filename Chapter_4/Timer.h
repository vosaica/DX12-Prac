#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ratio>

class Timer
{
    using TimePoint
        = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long long, std::micro>>;
    using Duration = std::chrono::duration<long long, std::micro>;

public:
    Timer();

    float TotalTime() const; // in seconds
    float DeltaTime() const; // in seconds

    void Reset(); // Call before message loop.
    void Start(); // Call when unpaused.
    void Stop();  // Call when paused.
    void Tick();  // Call every frame.

private:
    Duration mDeltaTime{};
    Duration mPausedTime{};

    TimePoint mBaseTime{};
    TimePoint mStopTime{};
    TimePoint mPrevTime{};
    TimePoint mCurrTime{};

    bool mStopped{};
};

#endif // TIMER_H
