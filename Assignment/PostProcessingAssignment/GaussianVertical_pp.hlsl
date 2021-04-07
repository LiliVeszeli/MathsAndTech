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
    float filter[7] = { 0.030078323, 0.104983664, 0.222250419, 0.285375187, 0.222250419, 0.104983664, 0.030078323 };
    
    float BlurWeights[13] =
    {
        0.002216,
   0.008764,
   0.026995,
   0.064759,
   0.120985,
   0.176033,
   0.199471,
   0.176033,
   0.120985,
   0.064759,
   0.026995,
   0.008764,
   0.002216,
    };
    
    float weights[21] =
    {
        0,
    0,
    0,
    0,
    0,
    0.000003,
    0.000229,
    0.005977,
    0.060598,
    0.24173,
    0.382925,
    0.24173,
    0.060598,
    0.005977,
    0.000229,
    0.000003,
    0,
    0,
    0,
    0,
    0,
    };
    //0.00296902
    //0.0133062
    //0.0219382
    //0.0133062
    //0.00296902

    //0.0133062
    //0.0596343
    //0.0983203
    //0.0596343
    //0.0133062

    //0.0219382
    //0.0983203
    //0.162103
    //0.0983203
    //0.0219382

    //0.0133062
    //0.0596343
    //0.0983203
    //0.0596343
    //0.0133062

    //0.00296902
    //0.0133062
    //0.0219382
    //0.0133062
    //0.00296902
    
	// Calculate alpha to display the effect in a softened circle, could use a texture rather than calculations for the same task.
	// Uses the second set of area texture coordinates, which range from (0,0) to (1,1) over the area being processed
    float softEdge = 0.20f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge); // Soft circle calculation based on fact that this circle has a radius of 0.5 (as area UVs go from 0->1)
    

    
    float3 colour = float3(0.0f, 0.0f, 0.0f);
    

    float offsetY = 1 / gViewportHeight* gGaussianStrength;
    colour += SceneTexture.Sample(PointSample, input.sceneUV) * weight[0];
    
    for (int i = 1; i < 5; ++i)
    {
        colour += SceneTexture.Sample(PointSample, input.sceneUV - float2(0, offsetY * i)) * weight[i];
        colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, offsetY * i)) * weight[i];
    }
    
    //colour += SceneTexture.Sample(PointSample, input.sceneUV - float2(0, offsetY));
    //colour += SceneTexture.Sample(PointSample, input.sceneUV + float2(0, offsetY));
     
    
  // colour /= 3;
	
	
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(colour, alpha);
}