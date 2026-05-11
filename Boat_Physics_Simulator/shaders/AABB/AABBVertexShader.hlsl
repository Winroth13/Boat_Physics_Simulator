// Input
struct VertexShaderInput
{
    float3 position : POSITION;
};

// Output
struct VertexShaderOutput
{
    float4 clipPosition : SV_POSITION;
    float3 color : COLOR;
};

cbuffer cbAabb : register(b0)
{
    float4x4 worldMatrix;
    float3 color;
    float pad0;
}

cbuffer cbPerView : register(b1)
{
    float4x4 viewProjMatrix;
    float4x4 viewMatrix;
    float3 cameraPos;
    float pad1;
}

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    float4 worldPos = mul(worldMatrix, float4(input.position.xyz, 1.0f));
    float4 clipPos = mul(viewProjMatrix, worldPos);

    output.clipPosition = clipPos;
    output.color = color;
    
    return output;
}