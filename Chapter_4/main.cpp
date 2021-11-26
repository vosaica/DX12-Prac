#include "PlatformHelpers.h"
#include "d3dApp.h"
#include "d3dx12.h"

#include <cassert>
#include <d3d12.h>
#include <debugapi.h>
#include <dxgi1_4.h>
#include <iostream>
#include <string>
#include <vector>
#include <wrl.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3D12.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace _4_1_8
{

void multiSample()
{
    ComPtr<ID3D12Device> md3dDevice;
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice)));

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels{};

    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                                  &msQualityLevels,
                                                  sizeof(msQualityLevels)));
}

} // namespace _4_1_8

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
        std::wstring text = L"Width = " + std::to_wstring(x.Width) + L" " + L"Height = "
                          + std::to_wstring(x.Height) + L" " + L"Refresh = " + std::to_wstring(n) + L"/"
                          + std::to_wstring(d) + L"\n";

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

namespace _4_3
{

void createDevice()
{
#ifndef NDEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> mdxgiFactory;
    ComPtr<ID3D12Device9> md3dDevice;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
    HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice));

    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter1> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice)));
    }

    ComPtr<ID3D12Fence> mFence;
    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    UINT mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    UINT mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    UINT mCbvUavDescriptorSize
        = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels{};
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R32G32B32_FLOAT;

    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                                  &msQualityLevels,
                                                  sizeof(msQualityLevels)));

    UINT m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    // assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level."); // 不知为何会触发

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
    ThrowIfFailed(md3dDevice->CreateCommandList(0,
                                                D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                mDirectCmdListAlloc.Get(),
                                                nullptr,
                                                IID_PPV_ARGS(mCommandList.GetAddressOf())));
    mCommandList->Close();
}

} // namespace _4_3

int main()
{
    // _4_1_10::LogAdapters();
    // _4_3::createDevice();

    return 0;
}