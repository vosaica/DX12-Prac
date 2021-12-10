#ifndef WIN32
#define WIN32
#endif // !WIN32

#include "D3DApp.h"
#include "DirectXTK12/SimpleMath.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <algorithm>
#include <array>
#include <d3d12.h>
#include <memory>
#include <vector>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = SimpleMath::Matrix::Identity;
};

class BoxApp : public D3DApp
{
public:
    BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
    ~BoxApp();

    virtual bool Initialize() override;
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    virtual void OnResize() override;
    virtual void Update(const Timer& gt) override;
    virtual void Draw(const Timer& gt) override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

    void CreateRtvAndDsvDescriptorHeaps() override;

private:
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = SimpleMath::Matrix::Identity;
    XMFLOAT4X4 mProj = SimpleMath::Matrix::Identity;
    XMFLOAT4X4 mView = SimpleMath::Matrix::Identity;

    float mTheta = 1.5F * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0F;

    POINT mLastMousePos{};
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
        BoxApp theAPP(hInstance);
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

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
    if (!D3DApp::Initialize())
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(mhMainWnd);
    ImGui_ImplDX12_Init(md3dDevice.Get(),
                        3,
                        DXGI_FORMAT_R8G8B8A8_UNORM,
                        mSrvHeap.Get(),
                        mSrvHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
                        mSrvHeap.Get()->GetGPUDescriptorHandleForHeapStart());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    FlushCommandQueue();

    return true;
}

void BoxApp::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25F * XM_PI, AspectRatio(), 1.0F, 1000.0F);
    XMStoreFloat4x4(&mProj, proj);
}

void BoxApp::Update(const Timer& gt)
{
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0F);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldView = world * view * proj;

    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldView));
    mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const Timer& gt)
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    auto barrier{CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                      D3D12_RESOURCE_STATE_PRESENT,
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET)};
    mCommandList->ResourceBarrier(1, &barrier);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(),
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                        1.0F,
                                        0,
                                        0,
                                        nullptr);

    auto currBackBuffer = CurrentBackBufferView();
    auto currDepthStencil = DepthStencilView();
    mCommandList->OMSetRenderTargets(1, &currBackBuffer, true, &currDepthStencil);

    ID3D12DescriptorHeap* descriptorHeaps[] = {mCbvHeap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto vbv = mBoxGeo->VertexBufferView();
    auto ibv = mBoxGeo->IndexBufferView();
    mCommandList->IASetVertexBuffers(0, 1, &vbv);
    mCommandList->IASetIndexBuffer(&ibv);
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                   D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier);

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its
    // code to learn more about Dear ImGui!).
    bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state

        ImGui::SliderFloat("float", &mPhi, 0.1f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f

        if (ImGui::Button(
                "Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::End();
    }
    mCommandList->SetDescriptorHeaps(1, mSrvHeap.GetAddressOf());
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25F * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25F * static_cast<float>(y - mLastMousePos.y));

        mTheta += dx;
        mPhi += dy;

        mPhi = std::clamp(mPhi, 0.1F, XM_PI - 0.1F);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.05F * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05F * static_cast<float>(y - mLastMousePos.y);

        mRadius += dx - dy;

        mRadius = std::clamp(mRadius, 5.0F, 150.0F);
    }
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

    UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * static_cast<unsigned long long>(objCBByteSize);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = CalcConstantBufferByteSize(sizeof(ObjectConstants));

    md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[1]{};

    CD3DX12_DESCRIPTOR_RANGE cbvTable{};
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1,
                                            slotRootParameter,
                                            0,
                                            nullptr,
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
                                             D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSig.GetAddressOf(),
                                             errorBlob.GetAddressOf());
    if (errorBlob != nullptr)
    {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(0,
                                                  serializedRootSig->GetBufferPointer(),
                                                  serializedRootSig->GetBufferSize(),
                                                  IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void BoxApp::BuildShadersAndInputLayout()
{
    mvsByteCode = CompileShader(L"Shaders/color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = CompileShader(L"Shaders/color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout = {
        {"POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {   "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices = {Vertex({XMFLOAT3(-1.0F, -1.0F, -1.0F), XMFLOAT4(Colors::White)}),
                                      Vertex({XMFLOAT3(-1.0F, +1.0F, -1.0F), XMFLOAT4(Colors::Black)}),
                                      Vertex({XMFLOAT3(+1.0F, +1.0F, -1.0F), XMFLOAT4(Colors::Red)}),
                                      Vertex({XMFLOAT3(+1.0F, -1.0F, -1.0F), XMFLOAT4(Colors::Green)}),
                                      Vertex({XMFLOAT3(-1.0F, -1.0F, +1.0F), XMFLOAT4(Colors::Blue)}),
                                      Vertex({XMFLOAT3(-1.0F, +1.0F, +1.0F), XMFLOAT4(Colors::Yellow)}),
                                      Vertex({XMFLOAT3(+1.0F, +1.0F, +1.0F), XMFLOAT4(Colors::Cyan)}),
                                      Vertex({XMFLOAT3(+1.0F, -1.0F, +1.0F), XMFLOAT4(Colors::Magenta)})};

    std::array<std::uint16_t, 36> indices = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
                                             3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7};

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    mBoxGeo->VertexBufferGPU = CreateDefaultBuffer(md3dDevice.Get(),
                                                   mCommandList.Get(),
                                                   vertices.data(),
                                                   vbByteSize,
                                                   mBoxGeo->VertexBufferUploader);
    mBoxGeo->IndexBufferGPU = CreateDefaultBuffer(md3dDevice.Get(),
                                                  mCommandList.Get(),
                                                  indices.data(),
                                                  ibByteSize,
                                                  mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride = sizeof(Vertex);
    mBoxGeo->VertexBufferByteSize = vbByteSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    mBoxGeo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mBoxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {mInputLayout.data(), (UINT)mInputLayout.size()};
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = {reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize()};
    psoDesc.PS = {reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void BoxApp::CreateRtvAndDsvDescriptorHeaps()
{
    D3DApp::CreateRtvAndDsvDescriptorHeaps();

    D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc{};
    SrvHeapDesc.NumDescriptors = 1;
    SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    SrvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(mSrvHeap.GetAddressOf())));
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT BoxApp::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }
    return D3DApp::MsgProc(hWnd, msg, wParam, lParam);
}