struct PixelShaderInput
{
    float4 clipPosition : SV_POSITION;
    float3 tint : TINT;
    float lifetime : LIFETIME;
    float2 uv : UV;
};

cbuffer PerParticleSystem : register(b0)
{
    uint ticks;
    float deltaTime;
    float lifetime;
    float spawnRadius;
    float velocity;
    float3 startTint;
    float3 endTint;
    uint flags;
    float atlasWidth;
    float atlasHeight;
    float animationSpeed;
    float desaturatePow;
    float startScale;
    float endScale;
    float2 pad0;
};

Texture2D diffuseTexture : register(t0);
Texture2D alphaTexture : register(t1);

SamplerState samplerState : register(s0);

/* Particle Flags */
#define IS_ANIMATED 1
#define IS_DESATURATE 2
#define RESET 4

float2 AtlasUV(float2 uv, float ratio)
{
    uint numTextures = uint(atlasWidth * atlasHeight);
    
    uint atlasIndex = uint((numTextures - 1) * ratio * animationSpeed) % numTextures;
    
    uint atlasX = atlasIndex % (uint) atlasWidth;
    uint atlasY = atlasIndex / (uint) atlasHeight;
    
    return float2(
        (uv.x + atlasX) / atlasWidth,
        (uv.y + atlasY) / atlasHeight
    );
}

float Luminance(float3 color)
{
    const float3 lumaWeight = float3(0.2125f, 0.7154f, 0.0721f);
    return dot(color, lumaWeight);
}

float3 Desaturate(float3 color)
{
    float luma = Luminance(color);
    return float3(luma.xxx);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float2 uv;
    float lifeRatio = input.lifetime / lifetime;
    
    if ((flags & IS_ANIMATED) == IS_ANIMATED)
    {
        uv = AtlasUV(input.uv, lifeRatio);
    }
    else
    {
        uv = input.uv;
    }
    
    /* Alpha check */
    float alpha = alphaTexture.Sample(samplerState, uv).r;
    if (alpha < 0.5f)
    {
        discard;
    }
    
    /* Diffuse */
    float3 color = diffuseTexture.Sample(samplerState, uv).rgb * input.tint;
    
    if ((flags & IS_DESATURATE) == IS_DESATURATE)
    {
        float3 desaturateColor = Desaturate(color);
        color = lerp(color, desaturateColor, pow(lifeRatio, abs(desaturatePow)));
    }
    
	return float4(color, 1.0f);
}