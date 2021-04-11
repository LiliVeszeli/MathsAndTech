

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
    
    float3 colour = float3(0.0f, 0.0f, 0.0f);
    
    float offsetX = 1 / gViewportWidth * gBlurStrength;
    float offsetY = 1 / gViewportHeight * gBlurStrength;

    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(-offsetX, -offsetY));
    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, -offsetY));
    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(+offsetX, -offsetY));

    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(-offsetX, 0));
    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, 0));
    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(+offsetX, 0));

    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(-offsetX, +offsetY));
    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, +offsetY));
    colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(+offsetX, +offsetY));

    colour /= 9;
    
   
    
    //tex.a = alpha;
    
    //if (gisMotionBlur == true)
    //{
    return gisMotionBlur ? float4(colour, 0.1f) : float4(colour, 1.0f);
    //}
    //else
    //{
    //    return float4(colour, 1.0f);
    //}  
}