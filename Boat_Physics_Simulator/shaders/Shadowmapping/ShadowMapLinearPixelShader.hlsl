// Input
struct PixelShaderInput
{
    float4 position : SV_POSITION;
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
    float dispStrength;
}

Texture2D alphaTexture : register(t7);

SamplerState samplerState : register(s0);

#define FAR_PLANE 100.0f

/* Material Flags */
#define HAS_NORMAL_MAP 1
#define HAS_DISPLACEMENT_MAP 2
#define HAS_ALPHA_MAP 4

float main(PixelShaderInput input) : SV_Target
{
    if ((materialFlags & HAS_ALPHA_MAP) == HAS_ALPHA_MAP)
    {
        float alpha = alphaTexture.Sample(samplerState, input.uv).r;
    
        if (alpha < 0.5f)
        {
            discard;
        }
    }

    float lightDistance = length(input.worldPosition - cameraPos);

    lightDistance = lightDistance / FAR_PLANE;
    
    return lightDistance;
}