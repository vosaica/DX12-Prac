#include "PlatformHelpers.h"

#include <d3d12.h>
#include <debugapi.h>
#include <dxgi.h>
#include <iostream>
#include <string>
#include <vector>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3D12.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

void _4_1_8()
{
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels{};

    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    // ThrowIfFailed(
    //     md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels,
    //     sizeof(msQualityLevels)));
}

namespace _4_1_10
{
void LogOutputDisplayModes(ComPtr<IDXGIOutput> output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // 用来获得符合条件的显示模式的个数
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (const auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text = L"Width = " + std::to_wstring(x.Width) + L" " + L"Height = " + std::to_wstring(x.Height) + L" "
                          + L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) + L"\n";

        OutputDebugString(text.c_str());
    }
}

void LogAdapterOutputs(ComPtr<IDXGIAdapter> adapter)
{
    UINT i = 0;
    ComPtr<IDXGIOutput> output;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L'\n';
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, DXGI_FORMAT_B8G8R8A8_UNORM);

        ++i;
    }
}

void LogAdapters()
{
    ThrowIfFailed(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
    ComPtr<IDXGIFactory> mdxgiFactory;
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mdxgiFactory)));
    UINT i = 0;
    ComPtr<IDXGIAdapter> adapter;
    std::vector<ComPtr<IDXGIAdapter>> adapterList;
    while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L'\n';

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        ++i;
    }
    for (const ComPtr<IDXGIAdapter>& adapter : adapterList)
    {
        LogAdapterOutputs(adapter);
    }
}

} // namespace _4_1_10

int main()
{
    _4_1_10::LogAdapters();

    return 0;
}