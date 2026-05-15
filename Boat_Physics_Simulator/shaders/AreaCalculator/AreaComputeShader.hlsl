Texture2D<float> tex : register(t0);
RWStructuredBuffer<float> areaBuffer : register(u0);

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint width, height;
    tex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    int2 pos = int2(DTid.xy);
    float color = tex[pos];

    areaBuffer[pos.y * width + pos.x] = color;
}