struct Particle
{
    float3 position;
    float3 tint;
    float scale;
    float lifetime;
};

struct VertexShaderOutput
{
    float3 worldPosition : WORLD_POSITION;
    float3 tint : TINT;
    float scale : SCALE;
    float lifetime : LIFETIME;
};

cbuffer cbPerObject : register(b2)
{
    float4x4 worldMatrix;
    float4x4 worldInvTransposeMatrix;
};

StructuredBuffer<Particle> Particles : register(t0);

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    VertexShaderOutput output;

    float4 worldPos = mul(float4(Particles[vertexID].position, 1.0f), worldMatrix);

    output.worldPosition = worldPos.xyz;
    output.scale = Particles[vertexID].scale;
    output.tint = Particles[vertexID].tint;
    output.lifetime = Particles[vertexID].lifetime;

	return output;
}