#include "Timer.h"

#include <cassert>
#include <chrono>
#include <ratio>

using namespace std::chrono;

Timer::Timer() : mDeltaTime(std::chrono::seconds(0)), mStopped(false)
{
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
double Timer::TotalTime() const
{
    // If we are stopped, do not count the time that has passed since we stopped.
    // Moreover, if we previously already had a pause, the distance
    // mStopTime - mBaseTime includes paused time, which we do not want to count.
    // To correct this, we can subtract the paused time from mStopTime:
    //
    //                     |<--paused time-->|
    // ----*---------------*-----------------*------------*------------*------> time
    //  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

    if (mStopped)
    {
        duration<double, std::ratio<1, 1>> dura1 = mStopTime - mBaseTime;
        return (dura1 - duration_cast<duration<double, std::ratio<1, 1>>>(mPausedTime)).count();
    }

    // The distance mCurrTime - mBaseTime includes paused time,
    // which we do not want to count.  To correct this, we can subtract
    // the paused time from mCurrTime:
    //
    //  (mCurrTime - mPausedTime) - mBaseTime
    //
    //                     |<--paused time-->|
    // ----*---------------*-----------------*------------*------> time
    //  mBaseTime       mStopTime        startTime     mCurrTime

    else
    {
        duration<double, std::ratio<1, 1>> dura1 = mCurrTime - mBaseTime;
        return (dura1 - duration_cast<duration<double, std::ratio<1, 1>>>(mPausedTime)).count();
    }
}

double Timer::DeltaTime() const
{
    return duration_cast<duration<double>>(mDeltaTime).count();
}

void Timer::Reset()
{
    TimePoint currTime = time_point_cast<Duration>(steady_clock::now());

    mBaseTime = currTime;
    mPrevTime = currTime;
    mStopTime = TimePoint{};
    mStopped = false;
}

void Timer::Start()
{
    auto startTime = time_point_cast<Duration>(steady_clock::now());
    // Accumulate the time elapsed between stop and start pairs.
    //
    //                     |<-------d------->|
    // ----*---------------*-----------------*------------> time
    //  mBaseTime       mStopTime        startTime

    if (mStopped)
    {
        mPausedTime += (startTime - mStopTime);

        mPrevTime = startTime;
        mStopTime = TimePoint{};
        mStopped = false;
    }
}

void Timer::Stop()
{
    if (!mStopped)
    {
        mStopTime = time_point_cast<Duration>(steady_clock::now());
        mStopped = true;
    }
}

void Timer::Tick()
{
    if (mStopped)
    {
        mDeltaTime = seconds(0);
        return;
    }

    mCurrTime = time_point_cast<Duration>(steady_clock::now());

    // Time difference between this frame and the previous.
    mDeltaTime = mCurrTime - mPrevTime;

    // Prepare for next frame.
    mPrevTime = mCurrTime;

    // Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the
    // processor goes into a power save mode or we get shuffled to another
    // processor, then mDeltaTime can be negative.
    // if (mDeltaTime < 0.0)
    // {
    //     mDeltaTime = 0.0;
    // }

    assert(mDeltaTime.count() >= 0);
}
