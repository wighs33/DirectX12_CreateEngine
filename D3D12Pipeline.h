#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl.h>

class D3D12RootSignature;

// 패스마다 다른 설정입니다. 나머지 설정은 모든 PSO가 같습니다.
struct PipelineSettings
{
    // Shader.hlsl에서 실행할 함수의 이름입니다.
    const char* vertexShader = nullptr;
    // 비워 두면 색을 쓰지 않고 깊이만 씁니다.
    const char* pixelShader = nullptr;
    // 그리지 않고 지울 면입니다.
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
};

// Shader와 그래픽 파이프라인 고정 설정을 PSO 하나로 묶습니다.
class D3D12Pipeline final
{
public:
    D3D12Pipeline() = default;
    ~D3D12Pipeline() = default;

    D3D12Pipeline(const D3D12Pipeline&) = delete;
    D3D12Pipeline& operator=(const D3D12Pipeline&) = delete;

    bool Initialize(
        ID3D12Device* device,
        const D3D12RootSignature& rootSignature,
        const PipelineSettings& settings);

    ID3D12PipelineState* Get() const;

private:
    bool CreatePipelineState(
        ID3D12Device* device,
        const D3D12RootSignature& rootSignature,
        const PipelineSettings& settings,
        ID3DBlob* vertexShader,
        ID3DBlob* pixelShader);

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};
