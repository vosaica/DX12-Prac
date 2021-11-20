#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <Windows.h>
#include <iostream>

using namespace DirectX;
using namespace DirectX::PackedVector;

std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v)
{
    XMFLOAT4 f4Dest;
    XMStoreFloat4(&f4Dest, v);

    os << "(" << f4Dest.x << " " << f4Dest.y << " " << f4Dest.z << " " << f4Dest.w << ")" << std::endl;
    return os;
}

// 一个XMMATRIX是四个XMVECTOR，所以在一个函数的FXMVECTOR不超过两个时，第一个XMMATRIX应该为FXMMATRIX，其余为CXMMATRIX
std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMMATRIX m)
{
    for (int i = 0; i < 4; ++i)
    {
        os << XMVectorGetX(m.r[i]) << '\t';
        os << XMVectorGetY(m.r[i]) << '\t';
        os << XMVectorGetZ(m.r[i]) << '\t';
        os << XMVectorGetW(m.r[i]) << '\n';
    }
    return os;
}

int main()
{
#ifndef NDEBUG
    std::clog << "Debugging!\n";
#endif // NDEBUG
    if (!XMVerifyCPUSupport())
    {
        std::cerr << "DirectXMath not supported on the CPU" << std::endl;
        return 0;
    }

    std::cout.setf(std::ios_base::boolalpha);
    std::cout.precision(8);

    auto mScaling = XMMatrixScaling(1.0F, 2.0F, 2.0F);
    auto mRotateX = XMMatrixRotationX(XM_PI);
    auto mTranslate = XMMatrixTranslation(10.0, 10.0, 10.0);

    auto vPointB = XMVectorSet(2.0F, 3.0F, 1.0F, 1.0F);
    std::cout << "PointB:\n" << vPointB << std::endl;
    // vW总是1
    std::cout << "Scaled PointB:\n" << XMVector3TransformCoord(vPointB, mScaling) << std::endl;
    std::cout << "Rotated PointB:\n" << XMVector3TransformCoord(vPointB, mRotateX) << std::endl;

    auto vVectorB = XMVectorSet(2.0F, 3.0F, 1.0F, 0.0F);
    std::cout << "VectorB:\n" << vVectorB << std::endl;
    // vW总是0
    std::cout << "Scaled PointB:\n" << XMVector3TransformNormal(vVectorB, mScaling) << std::endl;
    std::cout << "Rotated PointB:\n" << XMVector3TransformNormal(vVectorB, mRotateX) << std::endl;

    auto vPointA = XMVectorSet(2.0F, 3.0F, 2.0F, 2000.0F);
    std::cout << "PointA:\n" << vPointA << std::endl;
    // vW会被XMVector3TransformCoord改为1，所以不必显式设置vW
    std::cout << "Translated PointA:\n" << XMVector3TransformCoord(vPointA, mTranslate) << std::endl;

    return 0;
}