#ifndef WIN32
#define WIN32
#endif // !WIN32

#include "D3DApp.h"
#include "DirectXTK12/SimpleMath.h"

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

struct VPosData
{
    XMFLOAT3 Pos;
};

struct VColorData
{
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
    ~BoxApp() override;

    bool Initialize() override;

private:
    void OnResize() override;
    void Update(const Timer& gt) override;
    void Draw(const Timer& gt) override;

    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

    // Fields
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    std::unique_ptr<MeshGeometry<2>> mBoxGeo = nullptr;

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
    mMainWndCaption = L"Box App";
}

BoxApp::~BoxApp() = default;

bool BoxApp::Initialize()
{
    if (!D3DApp::Initialize())
    {
        return false;
    }

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
    mCommandList->OMSetRenderTargets(1, &currBackBuffer, TRUE, &currDepthStencil);

    ID3D12DescriptorHeap* descriptorHeaps[] = {mCbvHeap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto ibv = mBoxGeo->IndexBufferView();
    mCommandList->IASetVertexBuffers(0, 2, mBoxGeo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&ibv);
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                   D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier);

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
    mvsByteCode = CompileShader(L"Shaders/Chapter_6.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = CompileShader(L"Shaders/Chapter_6.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout = {
        {"POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {   "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<VPosData, 8> verticesPos = {VPosData{XMFLOAT3(-1.0F, -1.0F, -1.0F)},
                                           VPosData{XMFLOAT3(-1.0F, +1.0F, -1.0F)},
                                           VPosData{XMFLOAT3(+1.0F, +1.0F, -1.0F)},
                                           VPosData{XMFLOAT3(+1.0F, -1.0F, -1.0F)},
                                           VPosData{XMFLOAT3(-1.0F, -1.0F, +1.0F)},
                                           VPosData{XMFLOAT3(-1.0F, +1.0F, +1.0F)},
                                           VPosData{XMFLOAT3(+1.0F, +1.0F, +1.0F)},
                                           VPosData{XMFLOAT3(+1.0F, -1.0F, +1.0F)}};
    std::array<VColorData, 8> verticesColors = {VColorData{XMFLOAT4(Colors::White)},
                                                VColorData{XMFLOAT4(Colors::Black)},
                                                VColorData{XMFLOAT4(Colors::Red)},
                                                VColorData{XMFLOAT4(Colors::Green)},
                                                VColorData{XMFLOAT4(Colors::Blue)},
                                                VColorData{XMFLOAT4(Colors::Yellow)},
                                                VColorData{XMFLOAT4(Colors::Cyan)},
                                                VColorData{XMFLOAT4(Colors::Magenta)}};
    // clang-format off
    std::array<std::uint16_t, 36> indices = {// front
                                             0, 1, 2,
                                             0, 2, 3,

                                             // back
                                             4, 6, 5,
                                             4, 7, 6,

                                             // left
                                             4, 5, 1,
                                             4, 1, 0,

                                             // right
                                             3, 2, 6,
                                             3, 6, 7,

                                             // top
                                             1, 5, 6,
                                             1, 6, 2,

                                             // bottom
                                             4, 0, 3,
                                             4, 3, 7};
    // clang-format on
    const UINT vbByteSize0 = (UINT)verticesPos.size() * sizeof(VPosData);
    const UINT vbByteSize1 = (UINT)verticesColors.size() * sizeof(VColorData);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    mBoxGeo = std::make_unique<MeshGeometry<2>>();
    mBoxGeo->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize0, &mBoxGeo->VertexBufferCPU[0]));
    CopyMemory(mBoxGeo->VertexBufferCPU[0]->GetBufferPointer(), verticesPos.data(), vbByteSize0);

    mBoxGeo->VertexBufferGPU[0] = CreateDefaultBuffer(md3dDevice.Get(),
                                                      mCommandList.Get(),
                                                      verticesPos.data(),
                                                      vbByteSize0,
                                                      mBoxGeo->VertexBufferUploader[0]);

    ThrowIfFailed(D3DCreateBlob(vbByteSize1, &mBoxGeo->VertexBufferCPU[1]));
    CopyMemory(mBoxGeo->VertexBufferCPU[1]->GetBufferPointer(), verticesColors.data(), vbByteSize1);

    mBoxGeo->VertexBufferGPU[1] = CreateDefaultBuffer(md3dDevice.Get(),
                                                      mCommandList.Get(),
                                                      verticesColors.data(),
                                                      vbByteSize1,
                                                      mBoxGeo->VertexBufferUploader[1]);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    mBoxGeo->IndexBufferGPU = CreateDefaultBuffer(md3dDevice.Get(),
                                                  mCommandList.Get(),
                                                  indices.data(),
                                                  ibByteSize,
                                                  mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride[0] = sizeof(verticesPos);
    mBoxGeo->VertexBufferByteSize[0] = vbByteSize0;
    mBoxGeo->VertexByteStride[1] = sizeof(verticesColors);
    mBoxGeo->VertexBufferByteSize[1] = vbByteSize1;
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
