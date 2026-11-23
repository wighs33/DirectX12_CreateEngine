#include "D3D12RootSignature.h"

#include <cstdio>

#pragma comment(lib, "d3d12.lib")

using Microsoft::WRL::ComPtr;

bool D3D12RootSignature::Initialize(ID3D12Device* device)
{
    if (device == nullptr)
    {
        std::fprintf(stderr, "D3D12RootSignature::Initialize에 유효하지 않은 디바이스 포인터가 전달되었습니다.\n");
        return false;
    }

    // Pixel Shader의 register(t0)와 연결할 SRV Descriptor Range 하나를 정의합니다.
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[static_cast<UINT>(RootParameter::Count)]{};

    // 첫 번째 Parameter는 Vertex Shader의 register(b0)와 연결할 Object별 Root CBV입니다.
    D3D12_ROOT_PARAMETER& objectParameter =
        rootParameters[static_cast<UINT>(RootParameter::Object)];
    objectParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    objectParameter.Descriptor.ShaderRegister = 0;
    objectParameter.Descriptor.RegisterSpace = 0;
    objectParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // 두 번째 Parameter는 Shader Visible Heap의 Texture SRV를 가리키는 Descriptor Table입니다.
    D3D12_ROOT_PARAMETER& textureParameter =
        rootParameters[static_cast<UINT>(RootParameter::Texture)];
    textureParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    textureParameter.DescriptorTable.NumDescriptorRanges = 1;
    textureParameter.DescriptorTable.pDescriptorRanges = &srvRange;
    textureParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 세 번째 Parameter는 모든 Object가 공유하는 register(b1) Frame CBV입니다.
    // 외곽선의 Vertex Shader도 화면 비율과 행렬을 읽으므로 모든 Shader에 보입니다.
    D3D12_ROOT_PARAMETER& frameParameter =
        rootParameters[static_cast<UINT>(RootParameter::Frame)];
    frameParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameParameter.Descriptor.ShaderRegister = 1;
    frameParameter.Descriptor.RegisterSpace = 0;
    frameParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // 네 번째 Parameter는 조각의 재질마다 바뀌는 register(b2) Material CBV입니다.
    // 외곽선의 두께는 Vertex Shader가 읽으므로 모든 Shader에 보입니다.
    D3D12_ROOT_PARAMETER& materialParameter =
        rootParameters[static_cast<UINT>(RootParameter::Material)];
    materialParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    materialParameter.Descriptor.ShaderRegister = 2;
    materialParameter.Descriptor.RegisterSpace = 0;
    materialParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // 이번 단계에서는 변하지 않는 단일 Sampler를 Root Signature에 직접 포함합니다.
    D3D12_STATIC_SAMPLER_DESC staticSampler{};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // [10단계] WVP Root CBV와 Texture SRV, Static Sampler를 사용하는 Root Signature를 정의합니다.
    // Root Signature : Shader가 외부 Resource에 접근할 때 사용할 Resource Binding 구조와 규칙을 정의하는 객체
    D3D12_ROOT_SIGNATURE_DESC description{};
    description.NumParameters = _countof(rootParameters);
    description.pParameters = rootParameters;
    description.NumStaticSamplers = 1;
    description.pStaticSamplers = &staticSampler;
    description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Description을 Device가 읽을 수 있는 바이너리 Blob으로 직렬화합니다.
    // Shader Blob : CPU 메모리에 저장된 컴파일 결과, 이후 Pipeline State Object를 생성할 때 Shader Bytecode를 전달하는 데 사용
    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> errorBlob;
    const HRESULT serializationResult = D3D12SerializeRootSignature(
        &description,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSignature,
        &errorBlob);

    if (FAILED(serializationResult))
    {
        std::fprintf(
            stderr,
            "D3D12SerializeRootSignature 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(serializationResult));

        if (errorBlob != nullptr)
        {
            std::fwrite(
                errorBlob->GetBufferPointer(),
                1,
                errorBlob->GetBufferSize(),
                stderr);
            std::fputc('\n', stderr);
        }

        return false;
    }

    // 직렬화된 정의를 현재 Device에서 사용할 실제 Root Signature 객체로 생성합니다.
    const HRESULT creationResult = device->CreateRootSignature(
        0,
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    if (FAILED(creationResult))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateRootSignature 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(creationResult));
        return false;
    }

    return true;
}

ID3D12RootSignature* D3D12RootSignature::Get() const
{
    return rootSignature_.Get();
}
