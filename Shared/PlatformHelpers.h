//--------------------------------------------------------------------------------------
// File: PlatformHelpers.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#ifndef _PLATFORMHELPERS_
#define _PLATFORMHELPERS_

#pragma warning(disable : 4324)

#include "directx/d3dx12.h"

#include <D3Dcompiler.h>
#include <DirectXCollision.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <d3d12.h>
#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                                                        \
  (static_cast<uint32_t>(static_cast<uint8_t>(ch0)) | (static_cast<uint32_t>(static_cast<uint8_t>(ch1)) << 8) \
   | (static_cast<uint32_t>(static_cast<uint8_t>(ch2)) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(ch3)) << 24))
#endif /* defined(MAKEFOURCC) */

template <typename T, std::size_t N>
inline static decltype(auto) to_raw_array(std::array<T, N>& arr_v)
{
    return reinterpret_cast<T(&)[N]>(*arr_v.data());
}

namespace DirectX
{
// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
    com_exception(HRESULT hr) noexcept : result{hr}
    {
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        static std::array<char, 64> s_str = {};
        sprintf_s(to_raw_array(s_str), "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
        return s_str.data();
    }

    [[nodiscard]] HRESULT get_result() const noexcept
    {
        return result;
    }

private:
    HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr) noexcept(false)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}

// Helper for output debug tracing
inline void DebugTrace(_In_z_ _Printf_format_string_ const char* format, ...) noexcept
{
#ifdef _DEBUG
    va_list args;
    va_start(args, format);

    char buff[1024] = {};
    vsprintf_s(buff, format, args);
    OutputDebugStringA(buff);
    va_end(args);
#else
    UNREFERENCED_PARAMETER(format);
#endif
}

// Helper smart-pointers
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10) || (defined(_XBOX_ONE) && defined(_TITLE)) || !defined(WINAPI_FAMILY) \
    || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
struct virtual_deleter
{
    void operator()(void* p) noexcept
    {
        if (p != nullptr)
        {
            VirtualFree(p, 0, MEM_RELEASE);
        }
    }
};
#endif

struct handle_closer
{
    void operator()(HANDLE h) noexcept
    {
        if (h != nullptr)
        {
            CloseHandle(h);
        }
    }
};

using ScopedHandle = std::unique_ptr<void, handle_closer>;

inline HANDLE safe_handle(HANDLE h) noexcept
{
    return (h == INVALID_HANDLE_VALUE) ? nullptr : h;
}
} // namespace DirectX

namespace D3DUtils
{
inline std::wstring cstring2wstring(const char* str)
{
    size_t newsize = strlen(str) + 1;
    auto* cwstring = new wchar_t[newsize];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, cwstring, newsize, str, _TRUNCATE);
    std::wstring res{cwstring};
    delete[] cwstring;
    return res;
}

// Below are added Helpers from the d3dUtils.h
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device,
                                                           ID3D12GraphicsCommandList* cmdList,
                                                           const void* initData,
                                                           UINT64 byteSize,
                                                           Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

Microsoft::WRL::ComPtr<ID3DBlob> LoadShaderBinary(const std::wstring& filename);

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& filename,
                                               const D3D_SHADER_MACRO* defines,
                                               const std::string& entrypoint,
                                               const std::string& target);

inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

struct SubmeshGeometry
{
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    INT BaseVertexLocation = 0;

    DirectX::BoundingBox Bounds;
};

template <size_t N>
struct MeshGeometry
{
    std::string Name;

    std::array<Microsoft::WRL::ComPtr<ID3DBlob>, N> VertexBufferCPU{};
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU{};

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, N> VertexBufferGPU{};
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU{};

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, N> VertexBufferUploader{};
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader{};

    std::array<D3D12_VERTEX_BUFFER_VIEW, N> VertexBufferViews{};

    std::array<UINT, N> VertexByteStride{};
    std::array<UINT, N> VertexBufferByteSize{};
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW* VertexBufferView()
    {
        for (size_t i = 0; i < N; ++i)
        {
            VertexBufferViews[i].BufferLocation = VertexBufferGPU[i]->GetGPUVirtualAddress();
            VertexBufferViews[i].StrideInBytes = static_cast<UINT>(VertexByteStride[i]);
            VertexBufferViews[i].SizeInBytes = VertexBufferByteSize[i];
        }
        return VertexBufferViews.data();
    }

    [[nodiscard]] D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibv{};
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    void DisposeUploaders()
    {
        for (auto& i : VertexBufferUploader)
        {
            i = nullptr;
        }
        IndexBufferUploader = nullptr;
    }
};

template <typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        mIsConstantBuffer{isConstantBuffer},
        mElementByteSize{isConstantBuffer ? CalcConstantBufferByteSize(sizeof(T)) : sizeof(T)}
    {
        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data
        // at m*256 byte offsets and of n*256 byte lengths.
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256
        // UINT   SizeInBytes;   // multiple of 256
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;

        auto heapProperties{CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)};
        auto resourceDesc{CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(elementCount) * mElementByteSize)};
        DirectX::ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                               D3D12_HEAP_FLAG_NONE,
                                                               &resourceDesc,
                                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                                               nullptr,
                                                               IID_PPV_ARGS(&mUploadBuffer)));

        DirectX::ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
        {
            mUploadBuffer->Unmap(0, nullptr);
        }

        mMappedData = nullptr;
    }

    [[nodiscard]] ID3D12Resource* Resource() const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[static_cast<size_t>(elementIndex) * mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer{};
    BYTE* mMappedData{nullptr};

    ULONGLONG mElementByteSize{0};
    bool mIsConstantBuffer{false};
};

template <typename ObjectConstants, typename PassConstants>
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount) :
        PassCB{std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true)},
        ObjectCB{std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true)}
    {
        DirectX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CmdListAlloc)));
    }

    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource() = default;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc{};

    std::unique_ptr<UploadBuffer<PassConstants>> PassCB{};
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB{};

    UINT64 Fence{0};
};
} // namespace D3DUtils

#endif // _PLATFORMHELPERS_
