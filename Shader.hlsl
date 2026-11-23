// HLSL : High-Level Shader Language의 약자로, GPU에서 실행할 프로그램을 작성하는 언어

// C++ Root Signature의 첫 번째 Parameter가 가리키는 Vertex Shader용 b0 Constant Buffer입니다.
cbuffer TransformBuffer : register(b0)
{
    float4x4 worldViewProjection;
    float4x4 world;
    float4x4 worldInverseTranspose;
};

// 한 Frame 동안 모든 물체가 함께 쓰는 값입니다. SceneConstants의 FrameConstants와 배치가 같습니다.
cbuffer FrameBuffer : register(b1)
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float3 cameraPosition;
    float screenHeightOverWidth;
    float4x4 viewProjection;
    float4x4 lightViewProjection;
    float shadowDepthBias;
    float shadowNormalBias;
    float shadowTexelUv;
    float framePadding;
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
    float3 outlineColor;
    float outlineWidth;
};

// C++ Root Signature의 두 번째 Parameter와 Static Sampler가 각각 t0와 s0에 연결됩니다.
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

// 빛에서 본 깊이와, 그 깊이보다 가까운지 견준 결과를 섞어 주는 Sampler입니다.
Texture2D<float> shadowMap : register(t1);
SamplerComparisonState shadowSampler : register(s1);

// Input Assembler가 Vertex Buffer에서 읽어 Vertex Shader에 전달할 데이터입니다.
struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 outlineNormal : NORMAL1;
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

// 빛이 닿으면 1, 앞의 물체에 가려지면 0을 반환합니다.
float SampleShadow(float3 worldPosition)
{
    // 빛의 시점으로 옮긴 자리(-1~1)를 그림자 맵의 UV(0~1)로 바꿉니다. UV는 아래로 갈수록 커집니다.
    const float3 lightPosition =
        mul(float4(worldPosition, 1.0f), lightViewProjection).xyz;
    const float2 uv = lightPosition.xy * float2(0.5f, -0.5f) + 0.5f;

    // 두 칸씩 떨어진 열여섯 자리에서 견준 결과를 평균해 칸 모양의 계단을 없앱니다.
    // Sampler가 한 자리마다 이웃한 네 칸을 섞으므로 빈틈이 생기지 않습니다.
    float visibility = 0.0f;
    for (int y = -3; y <= 3; y += 2)
    {
        for (int x = -3; x <= 3; x += 2)
        {
            visibility += shadowMap.SampleCmpLevelZero(
                shadowSampler,
                uv + float2(x, y) * shadowTexelUv,
                saturate(lightPosition.z));
        }
    }
    visibility /= 16.0f;

    // 부드러워진 가장자리를 다시 좁혀 그림자도 두 단으로 끊습니다.
    return smoothstep(0.35f, 0.65f, visibility);
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
    const float facing = smoothstep(
        shadeThreshold - shadeSoftness,
        shadeThreshold + shadeSoftness,
        dot(normal, surfaceToLight));

    // 그림자가 진 곳도 그늘과 같은 단으로 두어 같은 그늘색이 되게 합니다.
    const float lit = facing * SampleShadow(input.worldPosition);

    // 그늘은 어둡게 누르지 않고 재질마다 정한 색을 곱합니다.
    float3 color = baseColor.rgb * lerp(shadeColor, 1.0f, lit) *
        lightColor.rgb * lightColor.w;
    color += baseColor.rgb * ambientColor.rgb * ambientWeight;

    // 시선과 거의 직각인 가장자리 중 빛을 받는 쪽에만 밝은 테두리를 더합니다.
    const float3 surfaceToCamera = normalize(
        cameraPosition - input.worldPosition);
    const float rim = step(
        1.0f - rimWidth,
        1.0f - saturate(dot(normal, surfaceToCamera))) * lit;
    color += rimColor * (rimStrength * rim);

    return float4(color, baseColor.a);
}

// 외곽선용 Vertex Shader : 정점을 화면에서 바깥쪽으로 정해진 픽셀만큼 밀어냅니다.
// 앞면은 PSO의 컬링으로 지우므로 부풀어 나온 뒷면만 테두리로 남습니다.
float4 VSOutline(VertexInput input) : SV_POSITION
{
    float4 position = mul(float4(input.position, 1.0f), worldViewProjection);

    // 밀어낼 방향을 화면 방향으로 바꿉니다. 투영 행렬이 줄여 둔 가로를 되돌려 픽셀 기준의 방향을 얻습니다.
    const float3 worldNormal = mul(
        float4(input.outlineNormal, 0.0f),
        worldInverseTranspose).xyz;
    float2 direction = mul(float4(worldNormal, 0.0f), viewProjection).xy;
    direction.x /= screenHeightOverWidth;
    direction = normalize(direction + float2(1.0e-5f, 0.0f));

    // 높이가 1080인 화면을 기준으로 한 픽셀 두께라서 해상도가 달라도 그림체가 같습니다.
    // 화면 좌표는 -1~1이므로 1픽셀은 2 / 1080이고, 나중에 w로 나눠지므로 미리 곱해 거리와 상관없게 합니다.
    float2 offset = direction * (outlineWidth * 2.0f / 1080.0f);
    offset.x *= screenHeightOverWidth;
    position.xy += offset * position.w;
    return position;
}

// 외곽선용 Pixel Shader : 빛과 상관없이 재질에 정한 색 하나로 칠합니다.
float4 PSOutline() : SV_TARGET
{
    return float4(outlineColor, 1.0f);
}

// 그림자용 Vertex Shader : 정점을 카메라 대신 빛의 시점으로 옮깁니다. Pixel Shader 없이 깊이만 남습니다.
float4 VSShadow(VertexInput input) : SV_POSITION
{
    float3 worldPosition = mul(float4(input.position, 1.0f), world).xyz;
    const float3 worldNormal = normalize(
        mul(float4(input.normal, 0.0f), worldInverseTranspose).xyz);
    const float3 lightTravel = normalize(lightDirection.xyz);

    // 빛의 반대쪽으로 조금, 비스듬한 면일수록 표면 안쪽으로 더 밀어 자기 그림자의 얼룩을 막습니다.
    const float slope = 1.0f - saturate(dot(worldNormal, -lightTravel));
    worldPosition += lightTravel * shadowDepthBias;
    worldPosition -= worldNormal * (slope * shadowNormalBias);
    return mul(float4(worldPosition, 1.0f), lightViewProjection);
}
