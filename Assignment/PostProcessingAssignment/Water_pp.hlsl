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

    input.sceneUV.x += sin(input.sceneUV.y * 3.14159 * 14 + gSWaterWiggle) / 300;
    
	
	// Calculate alpha to display the effect in a softened circle, could use a texture rather than calculations for the same task.
	// Uses the second set of area texture coordinates, which range from (0,0) to (1,1) over the area being processed
    float softEdge = 0.20f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge); // Soft circle calculation based on fact that this circle has a radius of 0.5 (as area UVs go from 0->1)
    
    float3 colour = float3(0.0f, 0.0f, 0.0f);
    
    float offsetX = 1 / gViewportWidth * 3;
    float offsetY = 1 / gViewportHeight * 3;

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
    
	
	// Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)
    colour *= gTintColourWater * 1.5;
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, 1.0f);
}