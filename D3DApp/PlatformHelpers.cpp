#include "PlatformHelpers.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ComPtr<ID3D12Resource> D3DUtils::CreateDefaultBuffer(ID3D12Device* device,
                                                     ID3D12GraphicsCommandList* cmdList,
                                                     const void* initData,
                                                     UINT64 byteSize,
                                                     ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    auto heapProperties{CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)};
    auto resourceDesc{CD3DX12_RESOURCE_DESC::Buffer(byteSize)};
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_COMMON,
                                                  nullptr,
                                                  IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                  nullptr,
                                                  IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    // On 64bit platform, only data size over 9EB may be overflow, so we can negelect it
    // On 32bit platform, the data size needs to be over 2GB
    subResourceData.RowPitch = static_cast<LONG_PTR>(byteSize);
    subResourceData.SlicePitch = subResourceData.RowPitch;

    auto barrier{CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                      D3D12_RESOURCE_STATE_COMMON,
                                                      D3D12_RESOURCE_STATE_COPY_DEST)};
    cmdList->ResourceBarrier(1, &barrier);
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                   D3D12_RESOURCE_STATE_COPY_DEST,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier);

    return defaultBuffer;
}

ComPtr<ID3DBlob> D3DUtils::LoadShaderBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

ComPtr<ID3DBlob> D3DUtils::CompileShader(const std::wstring& filename,
                                         const D3D_SHADER_MACRO* defines,
                                         const std::string& entrypoint,
                                         const std::string& target)
{
    UINT compileFlags = 0;
#ifndef NDEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(),
                            defines,
                            D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            entrypoint.c_str(),
                            target.c_str(),
                            compileFlags,
                            0,
                            &byteCode,
                            &errors);

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    return byteCode;
}
