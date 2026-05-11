struct PixelShaderInput
{
    float4 clipPosition : SV_POSITION;
    float2 uv : UV;
    float3 scale : SCALE;
    float3 tint : TINT;
};

Texture2D tex : register(t0);

SamplerState samplerState : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 color = tex.Sample(samplerState, input.uv);
    
    /* Alpha check */
    if (color.a < 0.5f)
    {
        discard;
    }
    
    return float4(color.rgb * input.tint, 1.0f);
}