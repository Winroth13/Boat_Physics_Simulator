// Input
struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

// Output
struct VertexShaderOutput
{
    float4 clipPosition : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
};

// Constant buffers
cbuffer cbPerView : register(b1)
{
    float4x4 viewProjMatrix;
    float4x4 viewMatrix;
    float3 cameraPos;
    float pad1;
}

cbuffer cbPerObject : register(b2)
{
    float4x4 worldMatrix;
    float4x4 worldInvTransposeMatrix;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    float4 worldPos = mul(float4(input.position.xyz, 1.0f), worldMatrix);
    float4 clipPos = mul(viewProjMatrix, worldPos);

    output.clipPosition = clipPos;
    output.worldPosition = worldPos.xyz;

    output.worldNormal = normalize(mul((float3x3) worldInvTransposeMatrix, input.normal));

    output.uv = input.uv;
    
    return output;
}