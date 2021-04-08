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
//--------------------------------------------------------------------------------------float3 rgb2hsv(float3 c)

float3 rgb2hsv(float3 c)
{
float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
float4 p = c.g < c.b ? float4(c.bg, K.wz) : float4(c.gb, K.xy);
float4 q = c.r < p.x ? float4(p.xyw, c.r) : float4(c.r, p.yzx);

float d = q.x - min(q.w, q.y);
float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 hsv2rgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}


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

    x += SceneTexture.Sample(PointSample, uv + float2(-texelSize.x, -texelSize.y)) * -1.0;
    x += SceneTexture.Sample(PointSample, uv + float2(-texelSize.x, 0)) * -2.0;
    x += SceneTexture.Sample(PointSample, uv + float2(-texelSize.x, texelSize.y)) * -1.0;
                                          
    x += SceneTexture.Sample(PointSample, uv + float2(texelSize.x, -texelSize.y)) * 1.0;
    x += SceneTexture.Sample(PointSample, uv + float2(texelSize.x, 0)) * 2.0;
    x += SceneTexture.Sample(PointSample, uv + float2(texelSize.x, texelSize.y)) * 1.0;
                                          
    y += SceneTexture.Sample(PointSample, uv + float2(-texelSize.x, -texelSize.y)) * -1.0;
    y += SceneTexture.Sample(PointSample, uv + float2(0, -texelSize.y)) * -2.0;
    y += SceneTexture.Sample(PointSample, uv + float2(texelSize.x, -texelSize.y)) * -1.0;
                                          
    y += SceneTexture.Sample(PointSample, uv + float2(-texelSize.x, texelSize.y)) * 1.0;
    y += SceneTexture.Sample(PointSample, uv + float2(0, texelSize.y)) * 2.0;
    y += SceneTexture.Sample(PointSample, uv + float2(texelSize.x, texelSize.y)) * 1.0;
    
   

    return sqrt(x * x + y * y);
}


// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float softEdge = 0.20f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge);
    
    float3 s = sobel(input.sceneUV);
    float3 tex = SceneTexture.Sample(PointSample, input.sceneUV);
    
    float3 hsvTex = rgb2hsv(tex);
    hsvTex.y = 1.0; // Modify saturation.
    hsvTex.z = 1.0; // Modify lightness/value.
    float3 colour = hsv2rgb(hsvTex);

    if (gisArea)
        return float4(colour * s, alpha);
    else
        return float4(colour * s, 1.0f);
}