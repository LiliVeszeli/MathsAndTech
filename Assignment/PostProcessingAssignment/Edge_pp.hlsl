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

float3 sobel(float2 uv)
{
    float x = 0;
    float y = 0;
    float2 texelSize = float2(1 / gViewportWidth, 1 / gViewportHeight);

    //float3 colour = float3(0.0f, 0.0f, 0.0f);
	
    //float e11 = SceneTexture.Sample(PointSample, ppIn.UV + float2(-PixelX, -PixelY));
    //float e12 = SceneTexture.Sample(PointSample, ppIn.UV + float2(0, -PixelY));
    //float e13 = SceneTexture.Sample(PointSample, ppIn.UV + float2(+PixelX, -PixelY));
              
    //float e21 = SceneTexture.Sample(PointSample, ppIn.UV + float2(-PixelX, 0));
    //float e22 = SceneTexture.Sample(PointSample, ppIn.UV + float2(0, 0));
    //float e23 = SceneTexture.Sample(PointSample, ppIn.UV + float2(+PixelX, 0));
                
    //float e31 = SceneTexture.Sample(PointSample, ppIn.UV + float2(-PixelX, +PixelY));
    //float e32 = SceneTexture.Sample(PointSample, ppIn.UV + float2(0, +PixelY));
    //float e33 = SceneTexture.Sample(PointSample, ppIn.UV + float2(+PixelX, +PixelY));

    x += SceneTexture.Sample(PointSample, uv  + float2(-texelSize.x, -texelSize.y)) * -1.0;
    x += SceneTexture.Sample(PointSample, uv  + float2(-texelSize.x, 0)) * -2.0;
    x += SceneTexture.Sample(PointSample, uv  + float2(-texelSize.x, texelSize.y)) * -1.0;
                                          
    x += SceneTexture.Sample(PointSample, uv  + float2(texelSize.x, -texelSize.y)) * 1.0;
    x += SceneTexture.Sample(PointSample, uv  + float2(texelSize.x, 0)) * 2.0;
    x += SceneTexture.Sample(PointSample, uv  + float2(texelSize.x, texelSize.y)) * 1.0;
                                          
    y += SceneTexture.Sample(PointSample, uv  + float2(-texelSize.x, -texelSize.y)) * -1.0;
    y += SceneTexture.Sample(PointSample, uv  + float2(0, -texelSize.y)) * -2.0;
    y += SceneTexture.Sample(PointSample, uv  + float2(texelSize.x, -texelSize.y)) * -1.0;
                                          
    y += SceneTexture.Sample(PointSample, uv  + float2(-texelSize.x, texelSize.y)) * 1.0;
    y += SceneTexture.Sample(PointSample, uv  + float2(0, texelSize.y)) * 2.0;
    y += SceneTexture.Sample(PointSample, uv + float2(texelSize.x, texelSize.y)) * 1.0;
    
   

    return sqrt(x * x + y * y);
}


// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{

    return float4(sobel(input.sceneUV), 1.0);
    
}