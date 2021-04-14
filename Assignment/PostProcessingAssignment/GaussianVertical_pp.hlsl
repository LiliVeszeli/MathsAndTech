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
    float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

    
 
    float weights[18] = { 0.033245, 0.0659162217, 0.0636705814, 0.0598194658, 0.0546642566, 0.0485871646, 0.0420045997, 0.0353207015, 0.0288880982, 0.0229808311, 0.0177815511, 0.013382297, 0.0097960001, 0.0069746748, 0.0048301008, 0.0032534598, 0.0021315311, 0.0013582974 };


    
    float3 colour = float3(0.0f, 0.0f, 0.0f);
    

    float offsetY = 1 / gViewportHeight* gGaussianStrength;
    colour += SceneTexture.Sample(PointSample, input.sceneUV) * weight[0];
    
    
    if (gBigger == false)
    {
        for (int i = 1; i < 5; ++i)
        {
            colour += SceneTexture.Sample(PointSample, input.sceneUV - float2(0, offsetY * i)) * weight[i];
            colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, offsetY * i)) * weight[i];
        }
    }
    else
    {
        for (int i = 1; i < 17; ++i)
        {
            colour += SceneTexture.Sample(PointSample, input.sceneUV - float2(0, offsetY * i)) * weights[i];
            colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, offsetY * i)) * weights[i];
        }
        colour *= 0.9;
    }
   
    
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, 1.0f);
}