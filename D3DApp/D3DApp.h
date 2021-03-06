#ifndef _D3DAPP_
#define _D3DAPP_

#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "../Shared/Timer.h"

#include <array>
#include <d3d12.h>
#include <debugapi.h>
#include <dxgi1_4.h>
#include <string>
#include <wrl.h>

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:
    D3DApp(HINSTANCE hInstance);
    virtual ~D3DApp();

public:
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;

    static D3DApp* GetApp();

    [[nodiscard]] HINSTANCE AppInst() const;
    [[nodiscard]] HWND MainWnd() const;
    [[nodiscard]] float AspectRatio() const;

    [[nodiscard]] bool Get4xMsaaState() const;
    void Set4xMsaaState(bool value);

    WPARAM Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptorHeaps();
    virtual void OnResize();
    virtual void Update(const Timer& gt) = 0;
    virtual void Draw(const Timer& gt) = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y)
    {
    }

    virtual void OnMouseUp(WPARAM btnState, int x, int y)
    {
    }

    virtual void OnMouseMove(WPARAM btnState, int x, int y)
    {
    }

    // Initialization and management of DirectX
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();

    void FlushCommandQueue();

    [[nodiscard]] ID3D12Resource* CurrentBackBuffer() const;
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(const Microsoft::WRL::ComPtr<IDXGIAdapter>& adapter);
    static void LogOutputDisplayModes(const Microsoft::WRL::ComPtr<IDXGIOutput>& output, DXGI_FORMAT format);

    // Fields
    static D3DApp* mApp;

    HINSTANCE mhAppInst{nullptr}; // application instance handle
    HWND mhMainWnd{nullptr};      // main window handle
    bool mAppPaused{false};       // is the application paused?
    bool mMinimized{false};       // is the application minimized?
    bool mMaximized{false};       // is the application maximized?
    bool mResizing{false};        // are the resize bars being dragged?
    bool mFullscreenState{false}; // fullscreen enabled

    // Set true to use 4X MSAA.  The default is false.
    bool m4xMsaaState{false}; // 4X MSAA enabled
    UINT m4xMsaaQuality{0};   // quality level of 4X MSAA

    // Used to keep track of the "delta-time" and game time.
    Timer mTimer{};

    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory{};
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice{};

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence{};
    UINT64 mCurrentFence = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue{};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc{};
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList{};

    static const int SwapChainBufferCount{2};
    int mCurrBackBuffer{};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, SwapChainBufferCount> mSwapChainBuffer{};
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer{};

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap{};

    D3D12_VIEWPORT mScreenViewport{};
    D3D12_RECT mScissorRect{};

    UINT mRtvDescriptorSize{};
    UINT mDsvDescriptorSize{};
    UINT mCbvSrvUavDescriptorSize{};

    // Derived class should set these in derived constructor to customize starting values.
    std::wstring mMainWndCaption{L"d3d App"};
    D3D_DRIVER_TYPE md3dDriverType{D3D_DRIVER_TYPE_HARDWARE};
    DXGI_FORMAT mBackBufferFormat{DXGI_FORMAT_R8G8B8A8_UNORM};
    DXGI_FORMAT mDepthStencilFormat{DXGI_FORMAT_D24_UNORM_S8_UINT};
    int mClientWidth{800};
    int mClientHeight{600};
};

#endif // _D3DAPP_
