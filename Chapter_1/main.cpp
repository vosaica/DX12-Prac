#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <Windows.h>
#include <iostream>

using namespace DirectX;
using namespace DirectX::PackedVector;

// Ϊ��ֱ�Ӱ�XMVECTOR�ŵ��Ĵ�������������Ҫ��XM_CALLCONV
// ǰ����FXMVECTOR, ���ĸ�GXMVECTOR�����塢����HXMVECTOR��ʣ�µ���CXMVECTOR
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
    const XMVECTORF32 vA{10.0F, 1.0F}; // �������������
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
