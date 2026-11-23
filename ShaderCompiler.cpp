#include "ShaderCompiler.h"

#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

bool ShaderCompiler::Compile(
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
