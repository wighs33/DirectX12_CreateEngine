// HLSL : High-Level Shader Language의 약자로, GPU에서 실행할 프로그램을 작성하는 언어

// Input Assembler가 Vertex Buffer에서 읽어 Vertex Shader에 전달할 데이터입니다.
struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

// Vertex Shader가 Rasterizer와 Pixel Shader로 전달할 데이터입니다.
struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Shader 실행이 시작되는 함수 : Vertex마다 실행
// Vertex 위치를 이미 Clip Space에 있다고 보고 float4로 확장하며 색상은 그대로 전달합니다.
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

// Shader 실행이 시작되는 함수 : Rasterizer가 만든 Pixel 후보마다 실행
// Rasterizer가 보간한 Vertex 색상을 Render Target의 최종 색상으로 출력합니다.
float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}
