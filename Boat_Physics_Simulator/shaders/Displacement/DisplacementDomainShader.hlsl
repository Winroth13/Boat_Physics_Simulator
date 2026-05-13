struct HullShaderOutput
{
    float3 worldPos : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct DomainShaderOutput
{
	float4 position  : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PatchConstantOutput
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

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

Texture2D displaceTexture : register(t6);

SamplerState samplerState : register(s0);

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DomainShaderOutput main(
	PatchConstantOutput input, 
	float3 barycentric : SV_DomainLocation,
	const OutputPatch<HullShaderOutput, NUM_CONTROL_POINTS> patch
)
{
    DomainShaderOutput output;

    /* World Position */
    output.worldPosition = 
        patch[0].worldPos * barycentric.x +
        patch[1].worldPos * barycentric.y + 
        patch[2].worldPos * barycentric.z;

    /* Normal */
    output.worldNormal = normalize(
        patch[0].worldNormal * barycentric.x +
        patch[1].worldNormal * barycentric.y +
        patch[2].worldNormal * barycentric.z
    );
    
    /* UV */
    output.uv =
        patch[0].uv * barycentric.x +
        patch[1].uv * barycentric.y +
        patch[2].uv * barycentric.z;
    
    // TODO: The displacement texture is RGB right now, becasue it breaks otherwise.
    float heightOffset = -0.5f + displaceTexture.SampleLevel(samplerState, output.uv, 0).r;
    heightOffset *= dispStrength;
    float3 displacedPos = output.worldPosition + output.worldNormal * heightOffset;
    
    output.position = mul(viewProjMatrix, float4(displacedPos, 1.0f));
    
    /* Tangent */
    output.tangent = normalize(
        patch[0].tangent * barycentric.x +
        patch[1].tangent * barycentric.y +
        patch[2].tangent * barycentric.z
    );

    /* Bitangent */
    output.bitangent = normalize(
        patch[0].bitangent * barycentric.x +
        patch[1].bitangent * barycentric.y +
        patch[2].bitangent * barycentric.z
    );
    
	return output;
}
