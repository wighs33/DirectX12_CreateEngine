#pragma once

#include <d3dcompiler.h>
#include <wrl.h>

// HLSL 파일에서 함수 하나를 골라 GPU가 실행할 코드로 바꿉니다.
class ShaderCompiler final
{
public:
    ShaderCompiler() = delete;

    // 실패하면 컴파일러가 알려 준 이유를 출력합니다.
    static bool Compile(
        const wchar_t* filePath,
        const char* entryPoint,
        const char* target,
        Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob);
};
