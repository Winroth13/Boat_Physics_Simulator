struct VertexShaderOutput
{
    float3 worldPosition : WORLD_POSITION;
    float3 tint : TINT;
    float scale : SCALE;
    float lifetime : LIFETIME;
};

struct GeometryShaderOutput
{
    float4 clipPosition : SV_POSITION;
    float3 tint : TINT;
    float lifetime : LIFETIME;
    float2 uv : UV;
};

cbuffer cbPerView : register(b1)
{
    float4x4 viewProjMatrix;
    float4x4 viewMatrix;
    float3 cameraPos;
    float pad1;
}

[maxvertexcount(6)]
void main(
	point VertexShaderOutput input[1], 
	inout TriangleStream<GeometryShaderOutput> output
)
{
    // Calculate vectors
    float3 upVect = normalize(viewProjMatrix._21_22_23);
    float3 rightVect = normalize(viewProjMatrix._11_12_13);

    // Scaling
    upVect = (upVect / 2.0f) * input[0].scale;
    rightVect = (rightVect / 2.0f) * input[0].scale;

    GeometryShaderOutput toAppend;
    toAppend.tint = input[0].tint;
    toAppend.lifetime = input[0].lifetime;
    // Top-left
    toAppend.clipPosition = mul(viewProjMatrix, float4(input[0].worldPosition - rightVect + upVect, 1.0f));
    toAppend.uv = float2(0.0f, 0.0f);
    output.Append(toAppend);
    // Bottom-right
    toAppend.clipPosition = mul(viewProjMatrix, float4(input[0].worldPosition + rightVect - upVect, 1.0f));
    toAppend.uv = float2(1.0f, 1.0f);
    output.Append(toAppend);
    // Bottom-left
    toAppend.clipPosition = mul(viewProjMatrix, float4(input[0].worldPosition - rightVect - upVect, 1.0f));
    toAppend.uv = float2(0.0f, 1.0f);
    output.Append(toAppend);

    output.RestartStrip();
    // Top-left
    toAppend.clipPosition = mul(viewProjMatrix, float4(input[0].worldPosition - rightVect + upVect, 1.0f));
    toAppend.uv = float2(0.0f, 0.0f);
    output.Append(toAppend);
    // Top-right
    toAppend.clipPosition = mul(viewProjMatrix, float4(input[0].worldPosition + rightVect + upVect, 1.0f));
    toAppend.uv = float2(1.0f, 0.0f);
    output.Append(toAppend);
    // Bottom-right
    toAppend.clipPosition = mul(viewProjMatrix, float4(input[0].worldPosition + rightVect - upVect, 1.0f));
    toAppend.uv = float2(1.0f, 1.0f);
    output.Append(toAppend);
}