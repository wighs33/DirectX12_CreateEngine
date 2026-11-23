// HLSL : High-Level Shader Language의 약자로, GPU에서 실행할 프로그램을 작성하는 언어

// C++ Root Signature의 첫 번째 Parameter가 가리키는 Vertex Shader용 b0 Constant Buffer입니다.
cbuffer TransformBuffer : register(b0)
{
    float4x4 worldViewProjection;
    float4x4 world;
    float4x4 worldInverseTranspose;
};

cbuffer LightingBuffer : register(b1)
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 cameraPosition;
};

// C++ Root Signature의 네 번째 Parameter가 가리키는 재질별 그림체 값입니다. Material.h의 MaterialConstants와 배치가 같습니다.
cbuffer MaterialBuffer : register(b2)
{
    float3 shadeColor;
    float shadeThreshold;
    float3 rimColor;
    float rimStrength;
    float shadeSoftness;
    float ambientWeight;
    float rimWidth;
    float materialPadding;
};

// C++ Root Signature의 두 번째 Parameter와 Static Sampler가 각각 t0와 s0에 연결됩니다.
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

// Input Assembler가 Vertex Buffer에서 읽어 Vertex Shader에 전달할 데이터입니다.
struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

// Vertex Shader가 Rasterizer와 Pixel Shader로 전달할 데이터입니다.
struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 worldNormal : NORMAL;
    float3 worldPosition : POSITION;
};

// Shader 실행이 시작되는 함수 : Vertex마다 실행
// Object Space 위치에 WVP Matrix를 적용해 Clip Space로 변환하고 색상은 그대로 전달합니다.
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProjection);
    output.color = input.color;
    output.uv = input.uv;
    output.worldNormal = normalize(
        mul(float4(input.normal, 0.0f), worldInverseTranspose).xyz);
    output.worldPosition = mul(float4(input.position, 1.0f), world).xyz;
    return output;
}

// Shader 실행이 시작되는 함수 : Rasterizer가 만든 Pixel 후보마다 실행
// Rasterizer가 보간한 UV로 Texture를 Sample하고 Vertex 색상을 곱한 바탕색에 2단 명암을 입혀 출력합니다.
// Texture는 읽을 때 선형 색이 되고 출력은 RTV가 sRGB로 바꾸므로 빛 계산은 모두 선형 색으로 합니다.
float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float4 textureColor = texture0.Sample(sampler0, input.uv);
    const float4 baseColor = textureColor * input.color;

    const float3 normal = normalize(input.worldNormal);
    const float3 surfaceToLight = normalize(-lightDirection.xyz);

    // 빛을 받는 정도를 밝은 면(1)과 그늘(0) 두 단으로 끊습니다. shadeSoftness는 경계가 번지는 폭입니다.
    const float lit = smoothstep(
        shadeThreshold - shadeSoftness,
        shadeThreshold + shadeSoftness,
        dot(normal, surfaceToLight));

    // 그늘은 어둡게 누르지 않고 재질마다 정한 색을 곱합니다.
    float3 color = baseColor.rgb * lerp(shadeColor, 1.0f, lit) *
        lightColor.rgb * lightColor.w;
    color += baseColor.rgb * ambientColor.rgb * ambientWeight;

    // 시선과 거의 직각인 가장자리 중 빛을 받는 쪽에만 밝은 테두리를 더합니다.
    const float3 surfaceToCamera = normalize(
        cameraPosition.xyz - input.worldPosition);
    const float rim = step(
        1.0f - rimWidth,
        1.0f - saturate(dot(normal, surfaceToCamera))) * lit;
    color += rimColor * (rimStrength * rim);

    return float4(color, baseColor.a);
}
