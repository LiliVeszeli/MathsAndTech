//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{    
    
    int radius = gRadius;
    float2 pixelSize = float2(1 / gViewportWidth, 1 / gViewportHeight);
    half2 uv = input.sceneUV;
    float3 mean[4] =
    {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 }
    };
 
    float3 sigma[4] =
    {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 }
    };
 
    float2 start[4] = { { -radius, -radius }, { -radius, 0 }, { 0, -radius }, { 0, 0 } };
 
    float2 pos;
    float3 col;
    for (int k = 0; k < 4; k++)
    {
        for (int i = 0; i <= radius; i++)
        {
            for (int j = 0; j <= radius; j++)
            {
                pos = float2(i, j) + start[k];
                col = SceneTexture.SampleLevel(PointSample, uv + float2(pos.x * pixelSize.x, pos.y * pixelSize.y), 10);
                mean[k] += col;
                sigma[k] += col * col;
            }
        }
    }
 
    float sigma2;
 
    float n = pow(radius + 1, 2);
    float3 colour = SceneTexture.Sample(PointSample, uv).rgb;
    float min = 1;
 
    for (int l = 0; l < 4; l++)
    {
        mean[l] /= n;
        sigma[l] = abs(sigma[l] / n - mean[l] * mean[l]);
        sigma2 = sigma[l].r + sigma[l].g + sigma[l].b;
 
        if (sigma2 < min)
        {
            min = sigma2;
            colour.rgb = mean[l].rgb;
        }
    }
    
    return float4(colour, 1.0f); 
}



