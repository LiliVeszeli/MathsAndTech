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


float4 main(PostProcessingInput input) : SV_Target
{

    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    float gamma = 0.6f;
    //float numColours = 10;
    
    colour = pow(colour, float3(gamma.rrr));
    colour = colour * gNumColours;
    colour = floor(colour);
    colour = colour / gNumColours;
    colour = pow(colour, float(1.0 / gamma));
    colour *= 1.2;

	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, 1.0f);
}



