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
	// Calculate alpha to display the effect in a softened circle, could use a texture rather than calculations for the same task.
	// Uses the second set of area texture coordinates, which range from (0,0) to (1,1) over the area being processed
    float softEdge = 0.20f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge); // Soft circle calculation based on fact that this circle has a radius of 0.5 (as area UVs go from 0->1)
    

    //float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    
    //float2 size = {gViewportWidth, gViewportHeight };
    //int radius = gRadius;
    //float2 _MainTex_TexelSize = float2(1 / gViewportWidth, 1 / gViewportHeight);
    //half2 uv = input.sceneUV;
    
    //float n = float((radius + 1) * (radius + 1));

    //float3 m[4];
    //float3 s[4];
    //for (int k = 0; k < 4; ++k)
    //{
    //    m[k] = float3(0.0, 0.0, 0.0);
    //    s[k] = float3(0.0, 0.0, 0.0);
    //}

    //for (int j = -radius; j <= 0; ++j)
    //{
    //    for (int i = -radius; i <= 0; ++i)
    //    {
    //        float3 c = SceneTexture.Sample(PointSample, uv + float2(i, j)/size).rgb;
    //        m[0] += c;
    //        s[0] += c * c;
    //    }
    //}

    //for (int j = -radius; j <= 0; ++j)
    //{
    //    for (int i = 0; i <= radius; ++i)
    //    {
    //        float3 c = SceneTexture.Sample(PointSample, uv + float2(i, j) / size).rgb;
    //        m[1] += c;
    //        s[1] += c * c;
    //    }
    //}

    //for (int j = 0; j <= radius; ++j)
    //{
    //    for (int i = 0; i <= radius; ++i)
    //    {
    //        float3 c = SceneTexture.Sample(PointSample, uv + float2(i, j) / size).rgb;
    //        m[2] += c;
    //        s[2] += c * c;
    //    }
    //}

    //for (int j = 0; j <= radius; ++j)
    //{
    //    for (int i = -radius; i <= 0; ++i)
    //    {
    //        float3 c = SceneTexture.Sample(PointSample, uv + float2(i, j) / size).rgb;
    //        m[3] += c;
    //        s[3] += c * c;
    //    }
    //}

    //float4 colour = float4(0, 0, 0, 0);
    //float min_sigma2 = 1e+2;
    //for (int k = 0; k < 4; ++k)
    //{
    //    m[k] /= n;
    //    s[k] = abs(s[k] / n - m[k] * m[k]);

    //    float sigma2 = s[k].r + s[k].g + s[k].b;
    //    if (sigma2 < min_sigma2)
    //    {
    //        min_sigma2 = sigma2;
    //        colour = float4(m[k], 1.0);
    //    }
        
    //}
 
    
    //return colour;
    
    
    int _Radius = gRadius;
    float2 _MainTex_TexelSize = float2(1 / gViewportWidth, 1 / gViewportHeight);
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
 
    float2 start[4] = { { -_Radius, -_Radius }, { -_Radius, 0 }, { 0, -_Radius }, { 0, 0 } };
 
    float2 pos;
    float3 col;
    for (int k = 0; k < 4; k++)
    {
        for (int i = 0; i <= _Radius; i++)
        {
            for (int j = 0; j <= _Radius; j++)
            {
                pos = float2(i, j) + start[k];
                col = SceneTexture.SampleLevel(PointSample, uv + float2(pos.x * _MainTex_TexelSize.x, pos.y * _MainTex_TexelSize.y), 10);                 
                mean[k] += col;
                sigma[k] += col * col;
            }
        }
    }
 
    float sigma2;
 
    float n = pow(_Radius + 1, 2);
    float3 color = SceneTexture.Sample(PointSample, uv).rgb;
    float min = 1;
 
    for (int l = 0; l < 4; l++)
    {
        mean[l] /= n;
        sigma[l] = abs(sigma[l] / n - mean[l] * mean[l]);
        sigma2 = sigma[l].r + sigma[l].g + sigma[l].b;
 
        if (sigma2 < min)
        {
            min = sigma2;
            color.rgb = mean[l].rgb;
        }
    }
    
    return float4(color, alpha);
    
}



