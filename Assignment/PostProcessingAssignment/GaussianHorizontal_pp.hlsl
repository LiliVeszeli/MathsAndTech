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

    float weight[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};
    
    
   // float offset[18] = { 0.0, 1.4953705027, 3.4891992113, 5.4830312105, 7.4768683759, 9.4707125766, 11.4645656736, 13.4584295168, 15.4523059431, 17.4461967743, 19.4661974725, 21.4627427973, 23.4592916956, 25.455844494, 27.4524015179, 29.4489630909, 31.445529535, 33.4421011704 };
   // float weights[18] = { 0.033245, 0.0659162217, 0.0636705814, 0.0598194658, 0.0546642566, 0.0485871646, 0.0420045997, 0.0353207015, 0.0288880982, 0.0229808311, 0.0177815511, 0.013382297, 0.0097960001, 0.0069746748, 0.0048301008, 0.0032534598, 0.0021315311, 0.0013582974 };

    
   
    float3 colour = float3(0.0f, 0.0f, 0.0f);
    
    float offsetX = 1 / gViewportWidth* gGaussianStrength;
    colour += SceneTexture.Sample(PointSample, input.sceneUV) * weight[0];
    
    
    for (int i = 1; i < 5; ++i)
    {
        colour += SceneTexture.Sample(PointSample, input.sceneUV - float2(offsetX * i, 0)) * weight[i];
        colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(offsetX * i, 0)) * weight[i];
    }

	
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, 1.0f);
}