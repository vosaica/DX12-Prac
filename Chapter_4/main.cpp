#include "GameTimer.h"
#include "Timer.h"
#include "d3dApp.h"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    GameTimer gt{};
    Timer t{};

    gt.Reset();
    t.Reset();
    gt.Start();
    t.Start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    gt.Stop();
    t.Stop();
    std::cout << gt.TotalTime() << std::endl;
    std::cout << t.TotalTime() << std::endl;
    gt.Start();
    t.Start();
    gt.Tick();
    t.Tick();

    std::cout << gt.DeltaTime() << std::endl;
    std::cout << t.DeltaTime() << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
    gt.Stop();
    t.Stop();
    std::cout << gt.TotalTime() << std::endl;
    std::cout << t.TotalTime() << std::endl;

    return 0;
}
