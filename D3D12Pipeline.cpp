#include "D3D12Pipeline.h"

#include "D3D12Config.h"
#include "D3D12RootSignature.h"
#include "ShaderCompiler.h"

#include <climits>
#include <cstdio>

using Microsoft::WRL::ComPtr;

bool D3D12Pipeline::Initialize(
    ID3D12Device* device,
    const D3D12RootSignature& rootSignature,
    const PipelineSettings& settings)
{
    // [11단계] 같은 HLSL 파일에서 Vertex Shader와 Pixel Shader를 각각 컴파일합니다.
    // Vertex Shader : Input Assembler가 전달한 Vertex 하나를 처리
    // Pixel Shader : Rasterizer가 생성한 Pixel 후보를 처리하고 RenderTarget에 기록할 최종 색상을 반환
    constexpr wchar_t shaderFilePath[] = L"Shader.hlsl";
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    if (!ShaderCompiler::Compile(
            shaderFilePath,
            settings.vertexShader,
            "vs_5_1",
            vertexShader))
    {
        return false;
    }
    if (settings.pixelShader != nullptr &&
        !ShaderCompiler::Compile(
            shaderFilePath,
            settings.pixelShader,
            "ps_5_1",
            pixelShader))
    {
        return false;
    }

    return CreatePipelineState(
        device,
        rootSignature,
        settings,
        vertexShader.Get(),
        pixelShader.Get());
}

ID3D12PipelineState* D3D12Pipeline::Get() const
{
    return pipelineState_.Get();
}

bool D3D12Pipeline::CreatePipelineState(
    ID3D12Device* device,
    const D3D12RootSignature& rootSignature,
    const PipelineSettings& settings,
    ID3DBlob* vertexShader,
    ID3DBlob* pixelShader)
{
    // [12단계] HLSL VertexInput의 POSITION, COLOR, TEXCOORD, NORMAL에 대응하는 Input Layout을 정의합니다.
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
        },
        {
            "NORMAL",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            // 외곽선을 밀어낼 방향입니다.
            "NORMAL",
            1,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        }
    };

    // Rasterizer는 채워진 Triangle을 그리고 패스가 정한 면을 제거합니다.
    // Rasterizer : Vertex Shader가 출력한 Primitive를 Pixel 후보로 변환
    D3D12_RASTERIZER_DESC rasterizerDescription{};
    rasterizerDescription.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDescription.CullMode = settings.cullMode;
    rasterizerDescription.FrontCounterClockwise = FALSE;
    rasterizerDescription.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDescription.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDescription.SlopeScaledDepthBias = settings.slopeScaledDepthBias;
    rasterizerDescription.DepthClipEnable = TRUE;
    rasterizerDescription.MultisampleEnable = settings.sampleCount > 1;
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
    description.pRootSignature = rootSignature.Get();
    description.VS.pShaderBytecode = vertexShader->GetBufferPointer();
    description.VS.BytecodeLength = vertexShader->GetBufferSize();
    if (pixelShader != nullptr)
    {
        description.PS.pShaderBytecode = pixelShader->GetBufferPointer();
        description.PS.BytecodeLength = pixelShader->GetBufferSize();
        description.NumRenderTargets = 1;
        description.RTVFormats[0] = D3D12Config::SceneTargetFormat;
    }
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
    description.DSVFormat = D3D12Config::DepthFormat;
    description.SampleDesc.Count = settings.sampleCount;
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
