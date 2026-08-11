#include "D3D12Pipeline.h"

#include <climits>
#include <cstdio>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

bool D3D12Pipeline::Initialize(ID3D12Device* device)
{
    if (device == nullptr)
    {
        std::fprintf(stderr, "D3D12Pipeline::Initialize에 유효하지 않은 디바이스 포인터가 전달되었습니다.\n");
        return false;
    }

    // 그래픽 파이프라인 준비 흐름 : Root Signature -> Shader -> Graphics PSO
    if (!CreateRootSignature(device))
        return false;
    if (!CreateShaders())
        return false;
    if (!CreatePipelineState(device))
        return false;

    return true;
}

ID3D12RootSignature* D3D12Pipeline::GetRootSignature() const
{
    return rootSignature_.Get();
}

ID3D12PipelineState* D3D12Pipeline::GetPipelineState() const
{
    return pipelineState_.Get();
}

bool D3D12Pipeline::CreateRootSignature(ID3D12Device* device)
{
    // Pixel Shader의 register(t0)와 연결할 SRV Descriptor Range 하나를 정의합니다.
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[2]{};

    // 첫 번째 Parameter는 Vertex Shader의 register(b0)와 연결할 Object별 Root CBV입니다.
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // 두 번째 Parameter는 Shader Visible Heap의 Texture SRV를 가리키는 Descriptor Table입니다.
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

bool D3D12Pipeline::CreateShaders()
{
    // [11단계] 같은 HLSL 파일에서 Vertex Shader와 Pixel Shader를 각각 컴파일합니다.
    // Vertex Shader : Input Assembler가 전달한 Vertex 하나를 처리
    // Pixel Shader : Rasterizer가 생성한 Pixel 후보를 처리하고 RenderTarget에 기록할 최종 색상을 반환
    constexpr wchar_t shaderFilePath[] = L"Shader.hlsl";

    if (!CompileShader(shaderFilePath, "VSMain", "vs_5_1", vertexShader_))
    {
        return false;
    }

    if (!CompileShader(shaderFilePath, "PSMain", "ps_5_1", pixelShader_))
    {
        return false;
    }

    return true;
}

bool D3D12Pipeline::CompileShader(
    const wchar_t* filePath,
    const char* entryPoint,
    const char* target,
    ComPtr<ID3DBlob>& shaderBlob)
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
    // Debug 빌드에서는 Shader 디버깅 정보를 포함하고 최적화를 생략합니다.
    compileFlags |= D3DCOMPILE_DEBUG;
    compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    // HLSL 파일에서 지정한 Entry Point를 찾아 특정 Shader Model 대상으로 컴파일합니다.
    ComPtr<ID3DBlob> errorBlob;
    const HRESULT result = D3DCompileFromFile(
        filePath,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob);

    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "D3DCompileFromFile 호출에 실패했습니다. 진입점: %s, 대상: %s, HRESULT: 0x%08X\n",
            entryPoint,
            target,
            static_cast<unsigned int>(result));

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

    return true;
}

bool D3D12Pipeline::CreatePipelineState(ID3D12Device* device)
{
    // [12단계] HLSL VertexInput의 POSITION, COLOR, TEXCOORD에 대응하는 Input Layout을 정의합니다.
    // Input Layout : Vertex Buffer의 바이트 데이터를 Vertex Shader 입력으로 해석하는 규칙
    const D3D12_INPUT_ELEMENT_DESC inputElements[] =
    {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "COLOR",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        }
    };

    // 기본 Rasterizer는 채워진 Triangle을 그리고 뒷면을 제거합니다.
    // Rasterizer : Vertex Shader가 출력한 Primitive를 Pixel 후보로 변환
    D3D12_RASTERIZER_DESC rasterizerDescription{};
    rasterizerDescription.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDescription.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDescription.FrontCounterClockwise = FALSE;
    rasterizerDescription.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDescription.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDescription.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDescription.DepthClipEnable = TRUE;
    rasterizerDescription.MultisampleEnable = FALSE;
    rasterizerDescription.AntialiasedLineEnable = FALSE;
    rasterizerDescription.ForcedSampleCount = 0;
    rasterizerDescription.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // 기본 Blend는 색상을 혼합하지 않고 Shader 출력값을 그대로 Render Target에 씁니다.
    D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlend{};
    renderTargetBlend.BlendEnable = FALSE;
    renderTargetBlend.LogicOpEnable = FALSE;
    renderTargetBlend.SrcBlend = D3D12_BLEND_ONE;
    renderTargetBlend.DestBlend = D3D12_BLEND_ZERO;
    renderTargetBlend.BlendOp = D3D12_BLEND_OP_ADD;
    renderTargetBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    renderTargetBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    renderTargetBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    renderTargetBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    renderTargetBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC blendDescription{};
    blendDescription.AlphaToCoverageEnable = FALSE;
    blendDescription.IndependentBlendEnable = FALSE;
    for (D3D12_RENDER_TARGET_BLEND_DESC& targetDescription : blendDescription.RenderTarget)
    {
        targetDescription = renderTargetBlend;
    }

    // 가까운 Pixel만 남도록 기본 LESS Depth Test와 Depth 쓰기를 활성화합니다.
    D3D12_DEPTH_STENCILOP_DESC stencilOperation{};
    stencilOperation.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    stencilOperation.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    stencilOperation.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    stencilOperation.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    D3D12_DEPTH_STENCIL_DESC depthStencilDescription{};
    depthStencilDescription.DepthEnable = TRUE;
    depthStencilDescription.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDescription.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDescription.StencilEnable = FALSE;
    depthStencilDescription.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDescription.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStencilDescription.FrontFace = stencilOperation;
    depthStencilDescription.BackFace = stencilOperation;

    // Root Signature, Shader와 고정 그래픽 파이프라인 설정을 하나의 PSO로 묶습니다.
    // Graphics Pipeline State Object : GPU의 그래픽 파이프라인 동작에 필요한 설정을 묶은 객체
    D3D12_GRAPHICS_PIPELINE_STATE_DESC description{};
    description.pRootSignature = rootSignature_.Get();
    description.VS.pShaderBytecode = vertexShader_->GetBufferPointer();
    description.VS.BytecodeLength = vertexShader_->GetBufferSize();
    description.PS.pShaderBytecode = pixelShader_->GetBufferPointer();
    description.PS.BytecodeLength = pixelShader_->GetBufferSize();
    // Blend State : Pixel Shader의 출력 색상을 RenderTarget에 이미 저장된 색상과 어떻게 합칠지 결정
    description.BlendState = blendDescription;
    description.SampleMask = UINT_MAX;
    description.RasterizerState = rasterizerDescription;
    description.DepthStencilState = depthStencilDescription;
    description.InputLayout.pInputElementDescs = inputElements;
    description.InputLayout.NumElements = _countof(inputElements);
    description.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    // PSO가 Triangle 계열 Primitive를 처리
    description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.NodeMask = 0;
    description.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    const HRESULT result = device->CreateGraphicsPipelineState(
        &description,
        IID_PPV_ARGS(&pipelineState_));

    if (FAILED(result))
    {
        std::fprintf(
            stderr,
            "ID3D12Device::CreateGraphicsPipelineState 호출에 실패했습니다. HRESULT: 0x%08X\n",
            static_cast<unsigned int>(result));
        return false;
    }

    return true;
}
