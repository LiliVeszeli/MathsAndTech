//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"

#define mod(x, y) (x - y * floor(x / y))
//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

Texture2D FrostTexture : register(t1);
SamplerState TrilinearWrap : register(s1);
//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float4 spline(float x, float4 c1, float4 c2, float4 c3, float4 c4, float4 c5, float4 c6, float4 c7, float4 c8, float4 c9)
{
    float w1, w2, w3, w4, w5, w6, w7, w8, w9;
    w1 = 0.0;
    w2 = 0.0;
    w3 = 0.0;
    w4 = 0.0;
    w5 = 0.0;
    w6 = 0.0;
    w7 = 0.0;
    w8 = 0.0;
    w9 = 0.0;
    float tmp = x * 8.0;
    if (tmp <= 1.0)
    {
        w1 = 1.0 - tmp;
        w2 = tmp;
    }
    else if (tmp <= 2.0)
    {
        tmp = tmp - 1.0;
        w2 = 1.0 - tmp;
        w3 = tmp;
    }
    else if (tmp <= 3.0)
    {
        tmp = tmp - 2.0;
        w3 = 1.0 - tmp;
        w4 = tmp;
    }
    else if (tmp <= 4.0)
    {
        tmp = tmp - 3.0;
        w4 = 1.0 - tmp;
        w5 = tmp;
    }
    else if (tmp <= 5.0)
    {
        tmp = tmp - 4.0;
        w5 = 1.0 - tmp;
        w6 = tmp;
    }
    else if (tmp <= 6.0)
    {
        tmp = tmp - 5.0;
        w6 = 1.0 - tmp;
        w7 = tmp;
    }
    else if (tmp <= 7.0)
    {
        tmp = tmp - 6.0;
        w7 = 1.0 - tmp;
        w8 = tmp;
    }
    else
    {

        tmp = clamp(tmp - 7.0, 0.0, 1.0);
        w8 = 1.0 - tmp;
        w9 = tmp;
    }
    return w1 * c1 + w2 * c2 + w3 * c3 + w4 * c4 + w5 * c5 + w6 * c6 + w7 * c7 + w8 * c8 + w9 * c9;
}


float3 NOISE2D(float2 uv)
{
    return FrostTexture.Sample(TrilinearWrap, uv).rgb;
}




float4 main(PostProcessingInput input) : SV_Target
{
    float PixelX = gpixX;
    float PixelY = gpixY;
    float Freq = gfreq;
    
    float2 uv = input.sceneUV;
    float3 tc = float3(1.0, 0.0, 0.0);
    
 
    float DeltaX = PixelX / gViewportWidth;
    float DeltaY = PixelY / gViewportHeight;
    float2 ox = float2(DeltaX, 0.0);
    float2 oy = float2(0.0, DeltaY);
    float2 PP = uv - oy;
    float4 C00 = SceneTexture.Sample(PointSample, PP - ox);
    float4 C01 = SceneTexture.Sample(PointSample, PP);
    float4 C02 = SceneTexture.Sample(PointSample, PP + ox);
    PP = uv;
    float4 C10 = SceneTexture.Sample(PointSample, PP - ox);
    float4 C11 = SceneTexture.Sample(PointSample, PP);
    float4 C12 = SceneTexture.Sample(PointSample, PP + ox);
    PP = uv + oy;
    float4 C20 = SceneTexture.Sample(PointSample, PP - ox);
    float4 C21 = SceneTexture.Sample(PointSample, PP);
    float4 C22 = SceneTexture.Sample(PointSample, PP + ox);
    
    float n = NOISE2D(Freq * uv).x;
    n = mod(n, 0.111111) / 0.111111;
    float4 result = spline(n, C00, C01, C02, C10, C11, C12, C20, C21, C22);
    tc = result.rgb *1.1;
  
  
    return float4(tc, 1.0f);

    
    //float DeltaX = (PixelX / gViewportWidth);
    //float DeltaY = (PixelY / gViewportHeight);
    //float2 ox = float2(DeltaX, 0.0);
    //float2 oy = float2(0.0, DeltaY);
    //float4 C00 = SceneTexture.Sample(PointSample, uv - oy - ox);
    //float4 C22 = SceneTexture.Sample(PointSample, uv + oy + ox);
    //float n = NOISE2D(Freq * uv).x;
    //n = fmod(n, 0.111111) / 0.111111;
    //float4 result = lerp(C00, C22, n);
    //return result;
}



