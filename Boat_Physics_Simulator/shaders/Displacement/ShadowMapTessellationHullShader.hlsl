struct VertexShaderOutput
{
    float4 clipPosition : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
};

struct HullShaderOutput
{
    float3 worldPos : WORLD_POSITION;
    float3 worldNormal : WORLD_NORMAL;
    float2 uv : UV;
};

struct PatchConstantOutput
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

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

#define NUM_CONTROL_POINTS 3

PatchConstantOutput CalcHSPatchConstants(InputPatch<VertexShaderOutput, NUM_CONTROL_POINTS> ip)
{
    PatchConstantOutput output;

    const float MIN_TESS_FACTOR = 1.0f;
    
    float3 objectPos = worldMatrix._41_42_43;
    float3 viewDir = objectPos - cameraPos;
    float distance = length(viewDir);
    float tessFalloffDistance = distance - maxTessDistance;
    float tessFalloffFactor = minTessDistance - maxTessDistance;
    
    float tessFactor = lerp(
        maxTessFactor,
        MIN_TESS_FACTOR,
        clamp(tessFalloffDistance / tessFalloffFactor, 0, 1)
    );

    output.EdgeTessFactor[0] = tessFactor;
    output.EdgeTessFactor[1] = tessFactor;
    output.EdgeTessFactor[2] = tessFactor;
    output.InsideTessFactor = tessFactor;

    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HullShaderOutput main(
    InputPatch<VertexShaderOutput, NUM_CONTROL_POINTS> ip,
    uint i : SV_OutputControlPointID
)
{
    HullShaderOutput output;

	output.worldPos = ip[i].worldPosition;
    output.worldNormal = ip[i].worldNormal;
    output.uv = ip[i].uv;
	
    return output;
}
