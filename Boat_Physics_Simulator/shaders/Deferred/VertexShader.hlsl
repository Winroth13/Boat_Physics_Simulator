// Input
struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
    float3 tangent : TANGENT;
};

// Output
struct VertexShaderOutput
{
    float4 clipPosition : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// Constant buffers
/*cbuffer cbPerFrame : register(b0)
{
    float3 sunDirection;
    float3 sunColor;
    float3 ambientColor;
    float3 pad0;
}*/

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

/*cbuffer cbPerMaterial : register(b3)
{
    float3 ambientCoefficient;
    float3 diffuseCoefficient;
    float3 specularCoefficient;
    float phongExponent;
    float2 padding;
}*/

VertexShaderOutput main(VertexShaderInput input)
{ 
    VertexShaderOutput output;
    
    float4 worldPos = mul(float4(input.position.xyz, 1.0f), worldMatrix);
    float4 clipPos = mul(viewProjMatrix, worldPos);

    output.clipPosition = clipPos;
    output.worldPosition = worldPos.xyz;

    output.worldNormal = normalize(mul((float3x3) worldInvTransposeMatrix, input.normal));
    output.uv = input.uv;
    
    float3 tangent = input.tangent;
    float3 normal = normalize(input.normal);
    
    output.tangent = normalize(tangent - dot(tangent, normal) * normal);
    output.bitangent = cross(normal, output.tangent);
    
    return output;
}