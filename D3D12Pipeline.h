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
    ID3D12PipelineState* GetPipelineState() const;

private:
    bool CreateRootSignature(ID3D12Device* device);
    bool CreateShaders();
    bool CreatePipelineState(ID3D12Device* device);
    bool CompileShader(
        const wchar_t* filePath,
        const char* entryPoint,
        const char* target,
        Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShader_;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};
