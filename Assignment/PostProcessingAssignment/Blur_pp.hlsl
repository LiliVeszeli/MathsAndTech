

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering
//SamplerState linearSampler : register(s1);

//static const float KernelOffsets[3] = { 0.0f, 1.3846153846f, 3.2307692308f };
//static const float BlurWeights[3] = { 0.2270270270f, 0.3162162162f, 0.0702702703f };



//cbuffer GaussianBlurConstantBuffer : register(b0)
//{
//    float2 textureDimensions; // The render target width/height.
//    float blurXOffset; // Controls how much of the render target is blurred along X axis [0.0. 1.0]. E.g. 1 = all of the RT is blurred, 0.5 = half of the RT is blurred, 0.0 = none of the RT is blurred.
//};

//cbuffer WorkloadConstantBuffer : register(b1)
//{
//    uint loopCount;
//};

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	
	// Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)
    //float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb * gTintColour;
	
    //float3 textureColor = float3(1.0f, 0.0f, 0.0f);
    //float2 uv = input.sceneUV;
    //if (uv.x > (blurXOffset + 0.005f))
    //{
    //    textureColor = SceneTexture.Sample(linearSampler, uv).xyz * BlurWeights[0];
    //    for (int i = 1; i < 3; i++)
    //    {
    //        float2 normalizedOffset = float2(0.0f, KernelOffsets[i]) / textureDimensions.y;
    //        textureColor += SceneTexture.Sample(linearSampler, uv + normalizedOffset).xyz * BlurWeights[i];
    //        textureColor += SceneTexture.Sample(linearSampler, uv - normalizedOffset).xyz * BlurWeights[i];
    //    }
    //}
    //else if (uv.x <= (blurXOffset - 0.005f))
    //{
    //    textureColor = SceneTexture.Sample(PointSample, uv).xyz;
    //}

    //// Artificially increase the workload to simulate a more complex shader.
    //const float3 textureColorOrig = textureColor;
    //for (uint i = 0; i < loopCount; i++)
    //{
    //    textureColor += textureColorOrig;
    //}

    //if (loopCount > 0)
    //{
    //    textureColor /= loopCount + 1;
    //}

    //return float4(textureColor, 1.0);
   
    float4 tex;
    tex = SceneTexture.Sample(PointSample, input.sceneUV.xy) * 0.6f;
    tex += SceneTexture.Sample(PointSample, input.sceneUV.xy + (0.004)) * 0.2f;
    
    float3 colour = float3(0.0f, 0.0f, 0.0f);
    
    float offsetX = 1 / gViewportWidth * 4;
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
    
    // Calculate alpha to display the effect in a softened circle, could use a texture rather than calculations for the same task.
	// Uses the second set of area texture coordinates, which range from (0,0) to (1,1) over the area being processed
    float softEdge = 0.20f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge);
    
    tex.a = alpha;
    
    return float4(colour, alpha);
    
}