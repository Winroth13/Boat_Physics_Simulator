// Input
struct VertexShaderInput
{
    float3 worldPosition : POSITION;
    float3 color : COLOR;
};

// Output
struct VertexShaderOutput
{
    float4 clipPosition : SV_POSITION;
    float3 color : COLOR;
};

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
    float4 clipPos = mul(viewProjMatrix, float4(input.worldPosition, 1.0f));

    output.clipPosition = clipPos;
    output.color = input.color;
    
    return output;
}