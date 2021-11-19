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

    os << "(" << f4Dest.x << " " << f4Dest.y << " " << f4Dest.z << " " << f4Dest.w << ")";
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

    XMFLOAT4X4 f4x4A{1.0F, 3.0F, 15.0F, 0.0F, 2.0F, 12.0F, 3.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    XMMATRIX mA = XMLoadFloat4x4(&f4x4A);
    XMMATRIX mB = XMMatrixSet(3.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F);
    XMVECTOR fDetB = XMMatrixDeterminant(mB);

    std::cout << "mA:\n" << mA << std::endl;
    std::cout << "mB:\n" << mB << std::endl;
    std::cout << "mB's transpose:\n" << XMMatrixTranspose(mB) << std::endl;
    std::cout << "mA * mB:\n" << mA * mB << std::endl;
    std::cout << "mB's det: " << fDetB << std::endl;
    std::cout << "mB^-1:\n" << XMMatrixInverse(&fDetB, mB) << std::endl;
    std::cout << "mB^-1 * mB:\n" << XMMatrixInverse(&fDetB, mB) * mB << std::endl;

    return 0;
}