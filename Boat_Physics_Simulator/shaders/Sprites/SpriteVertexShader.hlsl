struct VertexShaderInput
{
    float3 position : POSITION;
    float scale : SCALE;
    float3 tint : TINT;
};

struct VertexShaderOutput
{
    float3 worldPosition : WORLD_POSITION;
    float scale : SCALE;
    float3 tint : TINT;
};

cbuffer cbPerObject : register(b2)
{
    float4x4 worldMatrix;
    float4x4 worldInvTransposeMatrix;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.worldPosition = worldPos.xyz;
    output.scale = input.scale;
    output.tint = input.tint;

    return output;
}