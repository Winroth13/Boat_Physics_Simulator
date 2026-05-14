RWTexture2DArray<unorm float4> backBufferUAV;

Texture2D<float4> positionGBuffer : register(t11);
Texture2D<float4> normalGBuffer : register(t12);
Texture2D<float4> colorGBuffer : register(t13);

// Lights
struct DirectionalLight
{
    float4x4 viewProjMatrix;
    float3 direction;
    float intensity;
    float3 color;
    float pad0;
};

struct PointLight
{
    float3 position;
    float attenuation;
    float3 color;
    float intensity;
};

struct SpotLight
{
    float4x4 viewProjMatrix;
    float3 position;
    float attenuation;
    float3 color;
    float intensity;
    float3 direction;
    float angle;
};

// Material
struct Material
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
};

// Constant buffers
cbuffer cbPerFrame : register(b0)
{
    float3 ambientColor;
    uint numDirectionalLights;
    uint numPointLights;
    uint numSpotLights;
    uint flags;
    uint tickCount;
    uint2 screenDimensions;
    float2 pad0;
    float3 sceneCameraPos;
    float pad1;
};

cbuffer cbPerView : register(b1)
{
    float4x4 viewProjMatrix;
    float4x4 viewMatrix;
    float3 cameraPos;
    float pad2;
};

StructuredBuffer<DirectionalLight> directionalLights : register(t0);
StructuredBuffer<PointLight> pointLights : register(t1);
StructuredBuffer<SpotLight> spotLights : register(t2);

StructuredBuffer<Material> materials : register(t3);

Texture2DArray<float> directionalLightShadowMaps : register(t8);
TextureCubeArray<float> pointLightShadowMaps : register(t9);
Texture2DArray<float> spotLightShadowMaps : register(t10);

sampler shadowMapSampler : register(s1);

#define WIRE_FRAME 1
#define	SHOW_GBUFFERS 2
#define	USE_BLINN_PHONG 4

#define SHADOW_MAP_BIAS 0.01
#define SHADOW_SAMPLES_DIMENTIONS 6
#define SHADOW_OFFSET_STRENGTH 0.003f
#define DIRECTIONAL_SHADOW_OFFSET_STRENGTH 0.001f

#define FAR_PLANE 100.0f
#define LINEAR_SHADOW_MAP_BIAS 0.05
#define OMNI_SHADOW_SAMPLES 12
#define OMNI_SHADOW_DISK_RADIUS 0.01f
#define OMNI_SHADOW_OFFSET_STRENGTH 0.02f

float calcOmniShadowFactor(
    float3 fragmentWorldPosition,
    uint index,
    const TextureCubeArray<float> texArr
)
{
    float3 lightVector = fragmentWorldPosition - pointLights[index].position;
    float currentDepth = length(lightVector);

    float factor = 0.0f;
 
    const float3 sampleOffsetDirections[20] = 
    {   
        float3(1, 1, 1),    float3(1, -1, 1),   float3(-1, -1, 1),     float3(-1, 1, 1),
        float3(1, 1, -1),   float3(1, -1, -1),  float3(-1, -1, -1),    float3(-1, 1, -1),
        float3(1, 1, 0),    float3(1, -1, 0),   float3(-1, -1, 0),     float3(-1, 1, 0),
        float3(1, 0, 1),    float3(-1, 0, 1),   float3(1, 0, -1),      float3(-1, 0, -1),
        float3(0, 1, 1),    float3(0, -1, 1),   float3(0, -1, -1),     float3(0, 1, -1)
    };
    
    const static float3 sampleOffsets[15] =
    {
        float3(-0.9598, 0.2808, 0.6322), float3(-0.2244, -0.9745, 0.357), float3(-0.9229, 0.3851, -0.9767),
        float3(0.8927, 0.4506, 0.972), float3(0.3507, -0.9365, 0.994), float3(-0.2071, 0.9783, -0.978),
        float3(0.6478, 0.7618, -0.1473), float3(-0.61, 0.7924, 0.4793), float3(-0.3656, -0.9308, 0.376),
        float3(-0.1956, 0.9807, -0.8986), float3(0.8503, -0.5263, 0.7894), float3(-0.8202, -0.5721, -0.1578),
        float3(-0.301, 0.9536, 0.8495), float3(0.912, -0.4101, 0.1386), float3(0.6909, 0.723, 0.3322)
    };

    for (uint i = 0; i < OMNI_SHADOW_SAMPLES; ++i)
    {
        float3 offsetLightVector = lightVector + sampleOffsetDirections[i % 20] * OMNI_SHADOW_DISK_RADIUS + sampleOffsets[i % 15] * OMNI_SHADOW_OFFSET_STRENGTH;

        float closestDepth = texArr.SampleLevel(
            shadowMapSampler, 
            float4(normalize(offsetLightVector), index), 
            0
        );
        
        closestDepth *= FAR_PLANE;
        
        if (currentDepth > (closestDepth + LINEAR_SHADOW_MAP_BIAS))
        {
            factor += 0.0f;
        }
        else
        {
            factor += 1.0f;
        }
    }
    
    return (factor / OMNI_SHADOW_SAMPLES);
}

float calcShadowFactor(
    float3 fragmentWorldPosition,
    float4x4 lightViewProjMatrix,
    uint index,
    const Texture2DArray<float> texArr,
    bool isDirectional = false
)
{
    float offsetStrength;
    
    if (isDirectional)
        offsetStrength = DIRECTIONAL_SHADOW_OFFSET_STRENGTH;
    else
        offsetStrength = SHADOW_OFFSET_STRENGTH;
    
    const static float2 offsets[15] = 
    {
        float2(0.9268, 0.3755),     float2(0.5276, 0.8495),     float2(0.7642, -0.645),
        float2(-0.9908, -0.135),    float2(-0.1063, 0.9943),    float2(-0.999, 0.0447),
        float2(-0.9766, -0.2149),   float2(0.4578, 0.8891),     float2(0.5469, -0.8372),
        float2(-0.6466, 0.7629),    float2(0.2655, -0.9641),    float2(0.3806, 0.9247), 
        float2(-0.3756, -0.9268),   float2(0.7822, -0.6231),    float2(0.6246, -0.781)
    };
    
    float4 lightClipPos = mul(lightViewProjMatrix, float4(fragmentWorldPosition, 1.0f));
    float3 NDC = lightClipPos.xyz / lightClipPos.w;
    float currentDepth = NDC.z;
    
    float2 uv = float2((NDC.x * 0.5) + 0.5, (-NDC.y * 0.5) + 0.5);
    
    float factor = 0.0f;
    
    uint width = -1;
    uint height = -1;
    uint elements = -1;
    uint nLevels = -1;
    texArr.GetDimensions(index, width, height, elements, nLevels);
    
    uint numSamples = 0;
    
    for (int x = -SHADOW_SAMPLES_DIMENTIONS / 2; x <= SHADOW_SAMPLES_DIMENTIONS / 2; x++)
    {
        for (int y = -SHADOW_SAMPLES_DIMENTIONS / 2; y <= SHADOW_SAMPLES_DIMENTIONS / 2; y++)
        {
            uint offsetIndex = numSamples % 15;

            float2 realOffset = float2(x * (1.0f / width), y * (1.0f / height));
            realOffset += offsets[offsetIndex] * offsetStrength;
            float3 uvc = float3(uv + realOffset, index);
            float closestDepth = texArr.SampleLevel(shadowMapSampler, uvc, 0);
            if (currentDepth > (closestDepth + SHADOW_MAP_BIAS))
            {
                factor += 0.0f;
            }
            else
            {
                factor += 1.0f;
            }
            numSamples++;
        }
    }
    
    return (factor / numSamples);
}

float3 CalculateLightColor(
    float3 color,
    float intensity,
    float attenuation,
    float distance,
    float3 lightDir,
    float3 worldPosition,
    float3 worldNormal,
    float3 diffuseColor,
    uint materialIndex)
{
    float3 lightIntensity = color * intensity;
    attenuation = attenuation * pow(distance, 2);

    float3 viewDir = normalize(worldPosition - cameraPos);
    lightDir = normalize(lightDir);

    worldNormal = normalize(worldNormal);

    // coefficients and exponents
    float3 cDiffuse = materials[materialIndex].diffuseCoefficient * diffuseColor.rgb;
    float3 cSpecular = materials[materialIndex].specularCoefficient;

    // diffuse light
    float NdotL = dot(worldNormal, -lightDir);
    float diffuseIntensity = saturate(NdotL);
    float3 diffuseLight = cDiffuse * lightIntensity * diffuseIntensity / attenuation;

    float specularIntensity;
    // specular light
    if ((flags & USE_BLINN_PHONG) == USE_BLINN_PHONG)
    {
        // Blinn-Phong reflectance model
        float3 halfVect = normalize(-lightDir + -viewDir);
        float NdotH = dot(worldNormal, halfVect);
        specularIntensity = pow(saturate(NdotH), materials[materialIndex].phongExponent); // pow(0, 0) is undefined behaviour, but is prevented in Mesh.cpp
    }
    else
    {
        // Blinn reflectance model
        float3 reflectVect = reflect(lightDir, worldNormal);
        float RdotV = dot(reflectVect, -viewDir);
        specularIntensity = pow(saturate(RdotV), materials[materialIndex].phongExponent); // pow(0, 0) is undefined behaviour, but is prevented in Mesh.cpp
    }

    float3 specularLight = cSpecular * lightIntensity * specularIntensity / attenuation;
    float3 totalLight = diffuseLight + specularLight;
    return totalLight;
}

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID)
{   
    uint2 screenPos = DTid.xy;
    float2 screenUV = float2(DTid.x / (float) screenDimensions.x, DTid.y / (float) screenDimensions.y);

    float3 worldPosition = positionGBuffer[screenPos].xyz;
    float3 worldNormal = normalGBuffer[screenPos].xyz;
    float3 texColor = colorGBuffer[screenPos].rgb;
    uint materialIndex = colorGBuffer[screenPos].a;
    bool isSomething = positionGBuffer[screenPos].w;
    
    if ((flags & SHOW_GBUFFERS) == SHOW_GBUFFERS)
    {
        uint topY = screenPos.y;
        uint bottomY = screenPos.y - screenDimensions.y / 2;
        uint leftX = screenPos.x;
        uint rightX = screenPos.x - screenDimensions.x / 2;
        // Top-half
        if (screenUV.y < 0.5)
        {
            // Top left
            if (screenUV.x < 0.5)
            {
                backBufferUAV[uint3(screenPos, 0)] = float4(positionGBuffer[uint2(leftX, topY) * 2].rgb, 1.0f);
                return;
            }
            // Top right
            isSomething = positionGBuffer[uint2(rightX, topY) * 2].w;
            if (!isSomething)
            {
                backBufferUAV[uint3(screenPos, 0)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
                return;
            }
            backBufferUAV[uint3(screenPos, 0)] = float4(normalGBuffer[uint2(rightX, topY) * 2].xyz, 1.0f);
            return;
        }
        else if (screenUV.x < 0.5)
        {
            // Bottom left
            backBufferUAV[uint3(screenPos, 0)] = float4(colorGBuffer[uint2(leftX, bottomY) * 2].rgb, 1.0f);
            return;
        }
        // Bottom right
        uint2 offsetScreenPos = uint2(rightX, bottomY) * 2;
        worldPosition = positionGBuffer[offsetScreenPos].xyz;
        worldNormal = normalGBuffer[offsetScreenPos].xyz;
        texColor = colorGBuffer[offsetScreenPos].rgb;
        materialIndex = colorGBuffer[offsetScreenPos].a;
        isSomething = positionGBuffer[offsetScreenPos].w;
    }

    if (!isSomething)
    {
        return;
    }
    
    float3 totalLight;
    
    /* ambient */
    float3 iAmbient = ambientColor;
    float3 cAmbient = materials[materialIndex].ambientCoefficient * texColor.rgb;
    float3 ambientLight = cAmbient * iAmbient;
    
    totalLight = ambientLight;
    
    /* Directional lights */
    for (uint i = 0; i < numDirectionalLights; ++i)
    {
        float shadowFactor = calcShadowFactor(
            worldPosition,
            directionalLights[i].viewProjMatrix,
            i,
            directionalLightShadowMaps,
            true
        );
        
        if (shadowFactor != 0)
        {
            float3 color = CalculateLightColor(
                directionalLights[i].color,
                directionalLights[i].intensity,
                1,
                1,
                directionalLights[i].direction,
                worldPosition,
                worldNormal,
                texColor.rgb,
                materialIndex
            );
        
            totalLight += color * shadowFactor;
        }
    }
    
    /* Point lights */
    for (i = 0; i < numPointLights; ++i)
    {
        float shadowFactor = calcOmniShadowFactor(worldPosition, i, pointLightShadowMaps);
        
        if (shadowFactor != 0)
        {
            float3 color = CalculateLightColor(
                pointLights[i].color,
                pointLights[i].intensity,
                pointLights[i].attenuation,
                length(worldPosition - pointLights[i].position),
                worldPosition - pointLights[i].position,
                worldPosition,
                worldNormal,
                texColor.rgb,
                materialIndex
            );
        
            totalLight += color * shadowFactor;
        }
    }
    
    /* Spot lights */
    for (i = 0; i < numSpotLights; ++i)
    {
        float3 lightDir = (worldPosition - spotLights[i].position);
        float spotFactor = dot(normalize(lightDir), normalize(spotLights[i].direction));
        float cutoff = cos(spotLights[i].angle);

        if (spotFactor > cutoff)
        {
            float shadowFactor = calcShadowFactor(
                worldPosition,
                spotLights[i].viewProjMatrix,
                i,
                spotLightShadowMaps
            );

            if (shadowFactor != 0)
            {
                float3 color = CalculateLightColor(
                    spotLights[i].color,
                    spotLights[i].intensity,
                    spotLights[i].attenuation,
                    length(lightDir),
                    lightDir,
                    worldPosition,
                    worldNormal,
                    texColor.rgb,
                    materialIndex
                );

                // Smoothes out the outer 20% of the light
                // TODO: Add an outer angle to decide what the "outer percent" is
                float falloffFactor = (1.0 - (1.0 - spotFactor) / (1.0 - cutoff)) * 5;
                falloffFactor = min(falloffFactor, 1);

                color = color * falloffFactor * shadowFactor;

                totalLight += color;
            }
        }
    }

    /*float3 cameraToFrag = normalize(worldPosition - cameraPos);
    
    float depth = pointLightShadowMaps.SampleLevel(shadowMapSampler, float4(cameraToFrag, 1), 0);
    totalLight = float3(depth, depth, depth);*/
    
    backBufferUAV[uint3(screenPos, 0)] = float4(totalLight.rgb, 1.0f);
}