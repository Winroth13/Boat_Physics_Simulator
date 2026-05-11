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

struct Particle
{
    float3 position;
    float3 tint;
    float scale;
    float lifetime;
};

RWStructuredBuffer<Particle> Particles : register(u0);

#define PI 3.1415926538

/* Particle Flags */
#define IS_ANIMATED 1
#define IS_DESATURATE 2
#define ADDITIVE 4
#define RESET 8

/*
*   Pseudo-Random Number for GPU
*   Found here: https://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader
*/
float random(float2 p)
{
    float2 K1 = float2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfond-Schneider constant)
    );
    return frac(cos(dot(p, K1)) * 12345.6789);
}

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    Particle particle = Particles[DTid.x];

    float lifeRatio = particle.lifetime / lifetime;
    
    particle.lifetime += deltaTime;
    particle.position.y += velocity * deltaTime;
    
    if (particle.lifetime > lifetime || (flags & RESET) == RESET)
    {
        if ((flags & RESET) == RESET)
        {
            particle.lifetime = random(float2(DTid.x + ticks + 2, DTid.x + ticks + 2)) * lifetime;
        }
        else
        {
            particle.lifetime = particle.lifetime % lifetime; // This fixes the problem with dt jumping when moving the window
        }

        lifeRatio = particle.lifetime / lifetime;

        float angle = random(float2(DTid.x + ticks, DTid.x + ticks)) * PI * 2;
        float radiusRatio = random(float2(DTid.x + ticks + 1, DTid.x + ticks + 1));
        
        float x = cos(angle) * spawnRadius * radiusRatio;
        float z = sin(angle) * spawnRadius * radiusRatio;
        
        particle.position.x = x;
        particle.position.y = velocity * particle.lifetime;
        particle.position.z = z;
    }
    
    particle.scale = lerp(startScale, endScale, lifeRatio);
    particle.tint = lerp(startTint, endTint, lifeRatio);
    
    Particles[DTid.x] = particle;
}