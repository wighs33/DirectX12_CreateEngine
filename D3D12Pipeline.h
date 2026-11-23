#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl.h>

// Shader와 그래픽 파이프라인 고정 설정을 생성하고 함께 관리합니다.
class D3D12Pipeline final
{
public:
    D3D12Pipeline() = default;
    ~D3D12Pipeline() = default;

    D3D12Pipeline(const D3D12Pipeline&) = delete;
    D3D12Pipeline& operator=(const D3D12Pipeline&) = delete;

    bool Initialize(ID3D12Device* device);

    ID3D12RootSignature* GetRootSignature() const;
    // 밝은 면과 그늘을 두 단으로 그립니다.
    ID3D12PipelineState* GetToonPipelineState() const;
    // 부풀린 뒷면만 그려 테두리를 남깁니다.
    ID3D12PipelineState* GetOutlinePipelineState() const;

private:
    bool CreateRootSignature(ID3D12Device* device);
    bool CreateShaders();
    // Shader와 컬링만 다른 두 PSO를 만듭니다.
    bool CreatePipelineStates(ID3D12Device* device);
    static bool CreatePipelineState(
        ID3D12Device* device,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC& description,
        Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState);
    bool CompileShader(
        const wchar_t* filePath,
        const char* entryPoint,
        const char* target,
        Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3DBlob> toonVertexShader_;
    Microsoft::WRL::ComPtr<ID3DBlob> toonPixelShader_;
    Microsoft::WRL::ComPtr<ID3DBlob> outlineVertexShader_;
    Microsoft::WRL::ComPtr<ID3DBlob> outlinePixelShader_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> toonPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> outlinePipelineState_;
};
