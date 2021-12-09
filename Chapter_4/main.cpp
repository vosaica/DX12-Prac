#include "D3DApp.h"
#include "directx/d3dx12.h"

#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
    InitDirect3DApp(HINSTANCE hInstance);
    ~InitDirect3DApp() override;

    bool Initialize() override;

private:
    void OnResize() override;
    void Update(const Timer& t) override;
    void Draw(const Timer& t) override;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR pCmdLine,
                   _In_ int nShowCmd)
{
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        InitDirect3DApp theAPP(hInstance);
        if (!theAPP.Initialize())
        {
            return 0;
        }

        return theAPP.Run();
    }
    catch (com_exception& e)
    {
        auto message = cstring2wstring(e.what());
        MessageBox(nullptr, message.c_str(), L"HR Failed", MB_OK);

        return 0;
    }
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp() = default;

bool InitDirect3DApp::Initialize()
{
    return D3DApp::Initialize();
}

void InitDirect3DApp::OnResize()
{
    D3DApp::OnResize();
}

void InitDirect3DApp::Update(const Timer& t)
{
}

void InitDirect3DApp::Draw(const Timer& t)
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    auto barr = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                     D3D12_RESOURCE_STATE_PRESENT,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barr);

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(),
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                        1.0F,
                                        0,
                                        0,
                                        nullptr);

    auto currentBackBufferView = CurrentBackBufferView();
    auto depthStencilView = DepthStencilView();
    mCommandList->OMSetRenderTargets(1, &currentBackBufferView, TRUE, &depthStencilView);

    barr = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barr);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdsList), cmdsList);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}
