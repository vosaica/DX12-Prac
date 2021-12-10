#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <Windows.h>
#include <iostream>

using namespace DirectX;
using namespace DirectX::PackedVector;

// 为了直接把XMVECTOR放到寄存器，方法调用要加XM_CALLCONV
// 前三个FXMVECTOR, 第四个GXMVECTOR，第五、六个HXMVECTOR，剩下的用CXMVECTOR
std::ostream& XM_CALLCONV operator<<(std::ostream& os, FXMVECTOR v)
{
    XMFLOAT3 f3Dest;
    XMStoreFloat3(&f3Dest, v);

    os << "(" << f3Dest.x << " " << f3Dest.y << " " << f3Dest.z << ")";
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

    XMFLOAT2 f2A{12.0F, 10.0F};
    const XMVECTORF32 vA{{{10.0F, 1.0F}}}; // 常量类型用这个
    auto vB = XMLoadFloat2(&f2A);
    auto vC = vA * vB;

    auto fX = XMVectorGetX(vC);
    auto fY = XMVectorGetY(vC);
    auto fZ = XMVectorGetZ(vC);

    vC = XMVectorSetZ(vC, -2.0F);

    std::cout << fX << " " << fY << " " << fZ << " -> " << vC << std::endl;
    std::cout << "vC's length: " << XMVector3LengthEst(vC) << std::endl;
    std::cout << XMConvertToDegrees(XM_2PI) << std::endl;

    std::cout << XMVector3Length(XMVector3Normalize(vC)) << " " << XMVector3Length(XMVector3Normalize(vB));
    std::cout << " equal? : "
              << XMVector3NearEqual(XMVector3Length(XMVector3Normalize(vC)),
                                    XMVector3Length(XMVector3Normalize(vB)),
                                    XMVectorReplicate(0.0001))
              << std::endl;

    return 0;
}
