struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

#define WATER_LEVEL 0.0f

float4 main(PixelShaderInput input) : SV_TARGET
{
    if (input.worldPosition.y > WATER_LEVEL)
    {
        discard;
    }
    
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}