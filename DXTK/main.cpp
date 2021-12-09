#ifndef WIN32
#define WIN32
#endif // !WIN32

#include "DirectXTK12/SimpleMath.h"

#include <iostream>

using namespace DirectX::SimpleMath;

int main()
{
#ifndef NDEBUG
    std::cout << "Debugging!\n";
#endif // NDEBUG

    Vector2 vA{12.0F, 10.0F};
    std::cout << vA.x << " " << vA.y;

    return 0;
}
