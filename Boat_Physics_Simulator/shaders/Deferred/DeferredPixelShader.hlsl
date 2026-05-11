struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PixelShaderOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 color : SV_Target2;
};

// Constant buffers
cbuffer cbPerView : register(b1)
{
    float4x4 viewProjMatrix;
    float4x4 viewMatrix;
    float3 cameraPos;
    float pad0;
};

cbuffer cbPerMaterial : register(b3)
{
    float3 ambientCoefficient;
    float phongExponent;
    float3 diffuseCoefficient;
    float reflectiveness;
    float3 specularCoefficient;
    uint materialFlags;
    float maxTessFactor;
    float maxTessDistance;
    float minTessDistance;
    float pad1;
}

cbuffer cbMateralIndex : register(b4)
{
    uint materialIndex;
    float3 pad2;
};

Texture2D diffuseTexture : register(t3);
TextureCube cubemapTexture : register(t4);
Texture2D normalTexture : register(t5);
Texture2D alphaTexture : register(t7);
Texture2D dispTexture : register(t6);

SamplerState samplerState : register(s0);

/* Material Flags */
#define HAS_NORMAL_MAP 1
#define HAS_DISPLACEMENT_MAP 2
#define HAS_ALPHA_MAP 4

PixelShaderOutput main(PixelShaderInput input)
{
    if ((materialFlags & HAS_ALPHA_MAP) == HAS_ALPHA_MAP)
    {
        float alpha = alphaTexture.Sample(samplerState, input.uv).r;
        if (alpha < 0.5f)
        {
            discard;
        }
    }

    PixelShaderOutput output;
    output.position = float4(input.worldPosition, true);
    float3 resultingNormal;
    
    /* Normal Mapping */
    if ((materialFlags & HAS_NORMAL_MAP) == HAS_NORMAL_MAP)
    {
        float3x3 tbnMatrix = float3x3(
            normalize(input.tangent), 
            normalize(input.bitangent), 
            normalize(input.worldNormal)
        );
        float3 sampledNormal = normalTexture.Sample(samplerState, input.uv).rgb;
        sampledNormal *= 2.0f;
        sampledNormal -= float3(1.0f, 1.0f, 1.0f);

        resultingNormal = mul(sampledNormal, tbnMatrix);
    }
    else
    {
        resultingNormal = input.worldNormal;
    }
    
    output.normal = float4(resultingNormal, 1.0f);
    
    /* Diffuse */
    float3 color = diffuseTexture.Sample(samplerState, input.uv).rgb;
    
    /* Reflection */
    if (reflectiveness > 0)
    {
        float3 viewVector = input.worldPosition - cameraPos;
        float3 reflectVector = normalize(reflect(viewVector, resultingNormal));
        float3 reflectionColor = cubemapTexture.Sample(samplerState, reflectVector).rgb;
    
        color = lerp(color, reflectionColor, reflectiveness);
    }
    
    output.color = float4(
        color,
        materialIndex // Send material index to color gbuffer
    );
  
    return output;
}