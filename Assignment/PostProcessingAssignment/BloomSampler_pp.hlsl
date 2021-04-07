

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering
float3 rgb2hsv(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = c.g < c.b ? float4(c.bg, K.wz) : float4(c.gb, K.xy);
    float4 q = c.r < p.x ? float4(p.xyw, c.r) : float4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}


float Min(float fR, float fG, float fB)
{
    float fMin = fR;
    if (fG < fMin)
    {
        fMin = fG;
    }
    if (fB < fMin)
    {
        fMin = fB;
    }
    return fMin;
}


float Max(float fR, float fG, float fB)
{
    float fMax = fR;
    if (fG > fMax)
    {
        fMax = fG;
    }
    if (fB > fMax)
    {
        fMax = fB;
    }
    return fMax;
}

float3 RGBToHSL2(float3 rgb)
{
    float max = Max(rgb.r, rgb.g, rgb.b);
    float min = Min(rgb.r, rgb.g, rgb.b);
    float diff = max - min;
    
    float h;
    float s;
    float l;
    
    
    //HUE
    if (diff == 0)
    {
        h = 0;
    }
    else if (max == rgb.r)
    {
        h = 60 * fmod(((rgb.g - rgb.b) / diff), 6);
    }
    else if (max == rgb.g)
    {
        h = 60 * (((rgb.b - rgb.r) / diff) + 2);
    }
    else if (max == rgb.b)
    {
        h = 60 * (((rgb.r - rgb.g) / diff) + 4);
    }
    
    //LIGHT
    l = (max + min) / 2;
      
    
    //SAT
    if (diff == 0)
    {
        s = 0;
    }
    else
    {
        s = diff / (1 - abs(2 * l - 1));
    }
    
    return float3(h, s, l);
}


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
      
    float brightness = RGBToHSL2(colour).z;
    
    if (brightness < gThreshold)
    {
        colour = float3(0, 0, 0);
    }
    
    //(brightness > 0.7) ? float4(colour, 1.0f) : float4(0.0, 0.0, 0.0, 1.0)
    
    return float4(colour, 1);
}