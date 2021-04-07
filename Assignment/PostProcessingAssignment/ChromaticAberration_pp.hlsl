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

    //float3 colour = float3(0.0f, 0.0f, 0.0f);
    ////SceneTexture.Sample(PointSample, input.sceneUV).rgb;
	
    float offsetX = 1 / gViewportWidth;
    
    //float aberrationAmount = 0.1 + abs(gViewportHeight / 8.0);

    ////float2 uv = input.sceneUV.xy / gViewportHeight;
    //float2 distFromCenter = input.sceneUV - 0.5*offsetX;

    //// stronger aberration near the edges by raising to power 3
    //float2 aberrated = aberrationAmount * pow(distFromCenter, float2(3.0, 3.0));
    
    
    
    //colour.r = SceneTexture.Sample(PointSample, input.sceneUV - aberrated).r;
    //colour.g = SceneTexture.Sample(PointSample, input.sceneUV).g;
    //colour.b = SceneTexture.Sample(PointSample, input.sceneUV + aberrated).b;
    
    float4 red = SceneTexture.Sample(PointSample, float2(input.sceneUV.x - offsetX*5, input.sceneUV.y - offsetX*5)).r;
    float4 green = SceneTexture.Sample(PointSample, input.sceneUV).g;
    float4 blue = SceneTexture.Sample(PointSample, float2(input.sceneUV.x + offsetX*5, input.sceneUV.y + offsetX*5)).b;

    // Combine the offset colors.
    float3 colour = float3(red.r, green.g, blue.b);
    
    
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, 1.0f);
}