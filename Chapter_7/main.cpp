#include "../Shared/GeometryGenerator.h"
#include "../Shared/PlatformHelpers.h"
#include "D3DApp.h"
#include "DirectXTK12/SimpleMath.h"
#include "directx/d3dx12.h"

#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <algorithm>
#include <array>
#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;
using namespace D3DUtils;
using Microsoft::WRL::ComPtr;

struct Vertex
{
    XMFLOAT3 Pos{};
    XMFLOAT4 Color{};
};

struct ObjectConstants
{
    XMFLOAT4X4 World{SimpleMath::Matrix::Identity};
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View{SimpleMath::Matrix::Identity};
    DirectX::XMFLOAT4X4 InvView{SimpleMath::Matrix::Identity};
    DirectX::XMFLOAT4X4 Proj{SimpleMath::Matrix::Identity};
    DirectX::XMFLOAT4X4 InvProj{SimpleMath::Matrix::Identity};
    DirectX::XMFLOAT4X4 ViewProj{SimpleMath::Matrix::Identity};
    DirectX::XMFLOAT4X4 InvViewProj{SimpleMath::Matrix::Identity};
    DirectX::XMFLOAT3 EyePosW{0.0F, 0.0F, 0.0F};
    float cbPerObjectPad1{};
    DirectX::XMFLOAT2 RenderTargetSize{};
    DirectX::XMFLOAT2 InvRenderTargetSize{};
    float NearZ{};
    float FarZ{};
    float TotalTime{};
    float DeltaTime{};
};

class ShapesApp : public D3DApp
{
    using FrameResource = FrameResource<ObjectConstants, PassConstants>;
    using MeshGeometry = MeshGeometry<1>;

    struct RenderItem
    {
        RenderItem() = default;
        XMFLOAT4X4 World{Matrix::Identity};

        int NumFramesDirty{gNumFrameResources};
        UINT ObjCBIndex{0xffffffff};
        MeshGeometry* Geo{nullptr};

        D3D12_PRIMITIVE_TOPOLOGY PrimitiveType{D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST};

        UINT IndexCount{0};
        UINT StartIndexLocation{0};
        INT BaseVertexLocation{0};
    };

public:
    ShapesApp(HINSTANCE hInstance);
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;
    ~ShapesApp() override;

    bool Initialize() override;

private:
    void OnResize() override;
    void Update(const Timer& gt) override;
    void Draw(const Timer& gt) override;

    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;

    void UpdateObjectCBs(const Timer& gt);
    void UpdateMainPassCB(const Timer& gt);
    void OnKeyboardInput(const Timer& gt);
    void UpdateCamera(const Timer& gt);

    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

    void CreateCbvDescriptorHeaps();
    void BuildConstantBufferViews();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildRenderItems();
    void BuildPSOs();
    void BuildFrameResources();

    // Fields
    ComPtr<ID3D12RootSignature> mRootSignature{};
    ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap{};

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries{};
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders{};
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs{};

    static const UINT gNumFrameResources{3};
    std::vector<std::unique_ptr<FrameResource>> mFrameResources{};
    FrameResource* mCurrFrameResource{};
    UINT mCurrFrameResourceIndex{};
    UINT mPassCbvOffset{};

    PassConstants mMainPassCB{};

    std::vector<std::unique_ptr<RenderItem>> mAllRitems{};
    std::vector<RenderItem*> mOpaqueRitems{};
    std::vector<RenderItem*> mTransparentRitems{};

    bool mIsWireframe{false};

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout{};

    ComPtr<ID3D12PipelineState> mPSO{};

    XMFLOAT4X4 mProj{SimpleMath::Matrix::Identity};
    XMFLOAT4X4 mView{SimpleMath::Matrix::Identity};
    XMFLOAT3 mEyePos{};

    float mTheta{1.5F * XM_PI};
    float mPhi{XM_PIDIV4};
    float mRadius{5.0F};

    POINT mLastMousePos{};
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nShowCmd)
{
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        ShapesApp theAPP(hInstance);
        if (!theAPP.Initialize())
        {
            return 1;
        }

        return static_cast<int>(theAPP.Run());
    }
    catch (com_exception& e)
    {
        auto message{cstring2wstring(e.what())};
        MessageBox(nullptr, message.c_str(), L"HR Failed", MB_OK);

        return 1;
    }
}

ShapesApp::ShapesApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
    mMainWndCaption = L"Shapes App";
}

ShapesApp::~ShapesApp() = default;

bool ShapesApp::Initialize()
{
    if (!D3DApp::Initialize())
    {
        return false;
    }

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildRenderItems();
    BuildFrameResources();
    CreateCbvDescriptorHeaps();
    BuildConstantBufferViews();
    BuildPSOs();

    ThrowIfFailed(mCommandList->Close());
    std::array<ID3D12CommandList*, 1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());

    FlushCommandQueue();

    return true;
}

void ShapesApp::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj{XMMatrixPerspectiveFovLH(0.25F * XM_PI, AspectRatio(), 1.0F, 1000.0F)};
    XMStoreFloat4x4(&mProj, proj);
}

void ShapesApp::Update(const Timer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle{CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS)};
        if (eventHandle == nullptr)
        {
            std::string message{"Failed to create event: " + std::to_string(GetLastError())};
            throw std::runtime_error{message};
        }
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectCBs(gt);
    UpdateMainPassCB(gt);
}

void ShapesApp::UpdateObjectCBs(const Timer& gt)
{
    auto* currObjectCB{mCurrFrameResource->ObjectCB.get()};
    for (auto& e : mAllRitems)
    {
        if (e->NumFramesDirty > 0)
        {
            XMMATRIX world{XMLoadFloat4x4(&e->World)};

            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

            currObjectCB->CopyData(static_cast<int>(e->ObjCBIndex), objConstants);

            --e->NumFramesDirty;
        }
    }
}

void ShapesApp::UpdateMainPassCB(const Timer& gt)
{
    XMMATRIX view{XMLoadFloat4x4(&mView)};
    XMMATRIX proj{XMLoadFloat4x4(&mProj)};
    XMMATRIX viewProj{XMMatrixMultiply(view, proj)};

    auto viewDet{XMMatrixDeterminant(view)};
    auto projDet{XMMatrixDeterminant(proj)};
    auto vpDet{XMMatrixDeterminant(viewProj)};
    XMMATRIX invView{XMMatrixInverse(&viewDet, view)};
    XMMATRIX invProj{XMMatrixInverse(&projDet, proj)};
    XMMATRIX invViewProj{XMMatrixInverse(&vpDet, viewProj)};

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mEyePos;
    mMainPassCB.RenderTargetSize = XMFLOAT2(static_cast<float>(mClientWidth), static_cast<float>(mClientHeight));
    mMainPassCB.InvRenderTargetSize
        = XMFLOAT2(1.0F / static_cast<float>(mClientWidth), 1.0F / static_cast<float>(mClientHeight));
    mMainPassCB.NearZ = 1.0F;
    mMainPassCB.FarZ = 1000.0F;
    mMainPassCB.TotalTime = static_cast<float>(gt.TotalTime());
    mMainPassCB.DeltaTime = static_cast<float>(gt.DeltaTime());
}

void ShapesApp::Draw(const Timer& gt)
{
    auto& cmdListAlloc{mCurrFrameResource->CmdListAlloc};
    ThrowIfFailed(cmdListAlloc->Reset());

    if (mIsWireframe)
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
    }

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    auto barrier{CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                      D3D12_RESOURCE_STATE_PRESENT,
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET)};
    mCommandList->ResourceBarrier(1, &barrier);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList
        ->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, nullptr);

    auto backBufferView{CurrentBackBufferView()};
    auto depthStencilView{DepthStencilView()};
    mCommandList->OMSetRenderTargets(1, &backBufferView, TRUE, &depthStencilView);

    const std::array<ID3D12DescriptorHeap*, 1> descriptorHeaps{mCbvSrvUavHeap.Get()};
    mCommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCbvIndex{mPassCbvOffset + mCurrFrameResourceIndex};
    auto passCbvHandle{CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart())};
    passCbvHandle.Offset(static_cast<INT>(passCbvIndex), mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                   D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(mCommandList->Close());

    std::array<ID3D12CommandList*, 1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    mCurrFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void ShapesApp::OnKeyboardInput(const Timer& gt)
{
    mIsWireframe = (GetAsyncKeyState('1') & 0x8000) != 0;
}

void ShapesApp::UpdateCamera(const Timer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    mEyePos.y = mRadius * cosf(mPhi);

    // Build the view matrix.
    XMVECTOR pos{XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0F)};
    XMVECTOR target{XMVectorZero()};
    XMVECTOR up{XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F)};

    XMMATRIX view{XMMatrixLookAtLH(pos, target, up)};
    XMStoreFloat4x4(&mView, view);
}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    for (auto* ri : ritems)
    {
        cmdList->IASetVertexBuffers(0, 1, ri->Geo->VertexBufferView());
        auto ibv{ri->Geo->IndexBufferView()};
        cmdList->IASetIndexBuffer(&ibv);
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        UINT cbvIndex{mCurrFrameResourceIndex * static_cast<UINT>(mAllRitems.size()) + ri->ObjCBIndex};
        auto cbvHandle{CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart())};
        cbvHandle.Offset(static_cast<int>(cbvIndex), mCbvSrvUavDescriptorSize);

        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx{XMConvertToRadians(0.25F * static_cast<float>(x - mLastMousePos.x))};
        float dy{XMConvertToRadians(0.25F * static_cast<float>(y - mLastMousePos.y))};

        mTheta += dx;
        mPhi += dy;

        mPhi = std::clamp(mPhi, 0.1F, XM_PI - 0.1F);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx{0.05F * static_cast<float>(x - mLastMousePos.x)};
        float dy{0.05F * static_cast<float>(y - mLastMousePos.y)};

        mRadius += dx - dy;

        mRadius = std::clamp(mRadius, 5.0F, 150.0F);
    }
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void ShapesApp::BuildConstantBufferViews()
{
    UINT objCBByteSize{CalcConstantBufferByteSize(sizeof(ObjectConstants))};
    UINT objCount{static_cast<UINT>(mOpaqueRitems.size())};

    for (UINT frameIndex{0}; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto* objectCB{mFrameResources[frameIndex]->ObjectCB->Resource()};

        for (UINT i{0}; i < objCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress{objectCB->GetGPUVirtualAddress()};
            cbAddress += static_cast<unsigned long long>(i) * objCBByteSize;

            UINT heapIndex{frameIndex * objCount + i};
            auto handle{CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart())};
            handle.Offset(static_cast<int>(heapIndex), mCbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = objCBByteSize;

            md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    UINT passCBByteSize{CalcConstantBufferByteSize(sizeof(PassConstants))};

    for (UINT frameIndex{0}; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto* passCB{mFrameResources[frameIndex]->PassCB->Resource()};

        D3D12_GPU_VIRTUAL_ADDRESS cbAddress{passCB->GetGPUVirtualAddress()};

        auto heapIndex{mPassCbvOffset + frameIndex};
        auto handle{CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart())};
        handle.Offset(static_cast<INT>(heapIndex), mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;

        md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void ShapesApp::CreateCbvDescriptorHeaps()
{
    auto objCount{static_cast<UINT>(mAllRitems.size())};
    UINT numDesciptor{(objCount + 1) * gNumFrameResources};

    mPassCbvOffset = objCount * gNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = numDesciptor;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvSrvUavHeap)));
}

void ShapesApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE cbvTable0{};
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE cbvTable1{};
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    std::array<CD3DX12_ROOT_PARAMETER, 2> slotRootParameter{};

    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2,
                                            slotRootParameter.data(),
                                            0,
                                            nullptr,
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig{};
    ComPtr<ID3DBlob> errorBlob{};
    HRESULT hr{D3D12SerializeRootSignature(&rootSigDesc,
                                           D3D_ROOT_SIGNATURE_VERSION_1,
                                           serializedRootSig.GetAddressOf(),
                                           errorBlob.GetAddressOf())};
    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(0,
                                                  serializedRootSig->GetBufferPointer(),
                                                  serializedRootSig->GetBufferSize(),
                                                  IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void ShapesApp::BuildShadersAndInputLayout()
{
    mShaders["standardVS"] = CompileShader(L"Shaders\\Chapter_7.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = CompileShader(L"Shaders\\Chapter_7.hlsl", nullptr, "PS", "ps_5_1");

    mInputLayout = {
        {"POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {   "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

void ShapesApp::BuildShapeGeometry()
{
    GeometryGenerator::MeshData box{GeometryGenerator::CreateBox(1.5F, 0.5F, 1.5F, 3)};
    GeometryGenerator::MeshData grid{GeometryGenerator::CreateGrid(20.0F, 30.0F, 60, 40)};
    GeometryGenerator::MeshData sphere{GeometryGenerator::CreateSphere(0.5F, 20, 20)};
    GeometryGenerator::MeshData cylinder{GeometryGenerator::CreateCylinder(0.5F, 0.3F, 3.0F, 20, 20)};

    //
    // We are concatenating all the geometry into one big vertex/index buffer.  So
    // define the regions in the buffer each submesh covers.
    //

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT boxVertexOffset{0};
    UINT gridVertexOffset{static_cast<UINT>(box.Vertices.size())};
    UINT sphereVertexOffset{gridVertexOffset + static_cast<UINT>(grid.Vertices.size())};
    UINT cylinderVertexOffset{sphereVertexOffset + static_cast<UINT>(sphere.Vertices.size())};

    // Cache the starting index for each object in the concatenated index buffer.
    UINT boxIndexOffset{0};
    UINT gridIndexOffset{static_cast<UINT>(box.Indices32.size())};
    UINT sphereIndexOffset{gridIndexOffset + static_cast<UINT>(grid.Indices32.size())};
    UINT cylinderIndexOffset{sphereIndexOffset + static_cast<UINT>(sphere.Indices32.size())};

    // Define the SubmeshGeometry that cover different
    // regions of the vertex/index buffers.

    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = static_cast<UINT>(box.Indices32.size());
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = static_cast<INT>(boxVertexOffset);

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = static_cast<UINT>(grid.Indices32.size());
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = static_cast<INT>(gridVertexOffset);

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = static_cast<UINT>(sphere.Indices32.size());
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = static_cast<INT>(sphereVertexOffset);

    SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.IndexCount = static_cast<UINT>(cylinder.Indices32.size());
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = static_cast<INT>(cylinderVertexOffset);

    //
    // Extract the vertex elements we are interested in and pack the
    // vertices of all the meshes into one vertex buffer.
    //

    auto totalVertexCount{box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size()};

    std::vector<Vertex> vertices{totalVertexCount};

    UINT k{0};
    for (size_t i{0}; i < box.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
    }

    for (size_t i{0}; i < grid.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
    }

    for (size_t i{0}; i < sphere.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
    }

    for (size_t i{0}; i < cylinder.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
    }

    std::vector<std::uint16_t> indices{};
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

    const UINT vbByteSize{static_cast<UINT>(vertices.size() * sizeof(Vertex))};
    const UINT ibByteSize{static_cast<UINT>(indices.size() * sizeof(std::uint16_t))};

    auto geo{std::make_unique<MeshGeometry>()};
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU[0]));
    CopyMemory(geo->VertexBufferCPU[0]->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU[0]
        = CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader[0]);

    geo->IndexBufferGPU
        = CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride[0] = sizeof(Vertex);
    geo->VertexBufferByteSize[0] = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubmesh;
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;

    mGeometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc{};

    //
    // PSO for opaque objects.
    //
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = {mInputLayout.data(), (UINT)mInputLayout.size()};
    opaquePsoDesc.pRootSignature = mRootSignature.Get();
    opaquePsoDesc.VS
        = {reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize()};
    opaquePsoDesc.PS
        = {reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize()};
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
    opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    //
    // PSO for opaque wireframe objects.
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
}

void ShapesApp::BuildFrameResources()
{
    for (size_t i{0}; i < gNumFrameResources; ++i)
    {
        mFrameResources.emplace_back(
            std::make_unique<FrameResource>(md3dDevice.Get(), 1, static_cast<UINT>(mAllRitems.size())));
    }
}

void ShapesApp::BuildRenderItems()
{
    auto boxRitem{std::make_unique<RenderItem>()};
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0F, 2.0F, 2.0F) * XMMatrixTranslation(0.0F, 0.5F, 0.0F));
    boxRitem->ObjCBIndex = 0;
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mAllRitems.emplace_back(std::move(boxRitem));

    auto gridRitem{std::make_unique<RenderItem>()};
    gridRitem->World = Matrix::Identity;
    gridRitem->ObjCBIndex = 1;
    gridRitem->Geo = mGeometries["shapeGeo"].get();
    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
    mAllRitems.emplace_back(std::move(gridRitem));

    UINT objCBIndex{2};
    for (int i{0}; i < 5; ++i)
    {
        std::unique_ptr<RenderItem> leftCylRitem{std::make_unique<RenderItem>()};
        std::unique_ptr<RenderItem> rightCylRitem{std::make_unique<RenderItem>()};
        std::unique_ptr<RenderItem> leftSphereRitem{std::make_unique<RenderItem>()};
        std::unique_ptr<RenderItem> rightSphereRitem{std::make_unique<RenderItem>()};

        XMMATRIX leftCylWorld{XMMatrixTranslation(-5.0F, 1.5F, -10.0F + static_cast<float>(i) * 5.0F)};
        XMMATRIX rightCylWorld{XMMatrixTranslation(+5.0F, 1.5F, -10.0F + static_cast<float>(i) * 5.0F)};

        XMMATRIX leftSphereWorld{XMMatrixTranslation(-5.0F, 3.5F, -10.0F + static_cast<float>(i) * 5.0F)};
        XMMATRIX rightSphereWorld{XMMatrixTranslation(+5.0F, 3.5F, -10.0F + static_cast<float>(i) * 5.0F)};

        XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
        leftCylRitem->ObjCBIndex = objCBIndex++;
        leftCylRitem->Geo = mGeometries["shapeGeo"].get();
        leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
        rightCylRitem->ObjCBIndex = objCBIndex++;
        rightCylRitem->Geo = mGeometries["shapeGeo"].get();
        rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
        leftSphereRitem->ObjCBIndex = objCBIndex++;
        leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
        leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
        rightSphereRitem->ObjCBIndex = objCBIndex++;
        rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
        rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        mAllRitems.emplace_back(std::move(leftCylRitem));
        mAllRitems.emplace_back(std::move(rightCylRitem));
        mAllRitems.emplace_back(std::move(leftSphereRitem));
        mAllRitems.emplace_back(std::move(rightSphereRitem));
    }

    // All the render items are opaque.
    for (auto& e : mAllRitems)
    {
        mOpaqueRitems.push_back(e.get());
    }
}
