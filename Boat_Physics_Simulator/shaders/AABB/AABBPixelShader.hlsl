struct PixelShaderInput
{
    float4 clipPosition : SV_POSITION;
    float3 color : COLOR;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    return float4(input.color.rgb, 1.0f);
}