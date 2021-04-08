//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D    SceneTexture : register(t0);
SamplerState PointSample  : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

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

//float3 RGBToHSL(int R, int G, int B)
//{
//    int r = 100.0;
//    int g = 100.0;
//    int b = 200.0;

//    float fR = r / 255.0;
//    float fG = g / 255.0;
//    float fB = b / 255.0;
    
//    float H;
//    float S;
//    float L;


//    float fCMin = Min(fR, fG, fB);
//    float fCMax = Max(fR, fG, fB);


//    L = 50 * (fCMin + fCMax);

//    if (fCMin == fCMax)
//    {
//        S = 0;
//        H = 0;
//        //return;

//    }
//    else if (L < 50)
//    {
//        S = 100 * (fCMax - fCMin) / (fCMax + fCMin);
//    }
//    else
//    {
//        S = 100 * (fCMax - fCMin) / (2.0 - fCMax - fCMin);
//    }

//    if (fCMax == fR)
//    {
//        H = 60 * (fG - fB) / (fCMax - fCMin);
//    }
//    if (fCMax == fG)
//    {
//        H = 60 * (fB - fR) / (fCMax - fCMin) + 120;
//    }
//    if (fCMax == fB)
//    {
//        H = 60 * (fR - fG) / (fCMax - fCMin) + 240;
//    }
//    if (H < 0)
//    {
//        H = H + 360;
//    }
    
//    return float3(H/255, S/255, L/255);

//}

float Epsilon = 1e-10;
 
float3 RGBtoHCV(in float3 RGB)
{
    // Based on work by Sam Hocevar and Emil Persson
    float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0 / 3.0) : float4(RGB.gb, 0.0, -1.0 / 3.0);
    float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return float3(H, C, Q.x);
}


float3 RGBtoHSL(in float3 RGB)
{
    float3 HCV = RGBtoHCV(RGB);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
    return float3(HCV.x, S, L);
}

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 HSLtoRGB(in float3 HSL)
{
    float3 RGB = HUEtoRGB(HSL.x);
    float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
    return (RGB - 0.5) * C + HSL.z;
}

//float3 rgbToHsl(float r, float g, float b)
//{
//   // r /= 255, g /= 255, b /= 255;
//    float max = Max(r, g, b), min = Min(r, g, b);
//    float h, s, l = (max + min) / 2;

//    if (max == min)
//    {
//        h = s = 0; // achromatic
//    }
//    else
//    {
//        float d = max - min;
//        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
//        switch (max)
//        {
//            case r:
//                h = (g - b) / d + (g < b ? 6 : 0);
//                break;
//            case g:
//                h = (b - r) / d + 2;
//                break;
//            case b:
//                h = (r - g) / d + 4;
//                break;
//        }
//        h /= 6;
//    }

//    return float3(h, s, l);
//}
//function based on https://www.rapidtables.com/convert/color/rgb-to-hsl.html
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
    l = (max + min)/2;
      
    
    //SAT
    if (diff == 0)
    {
        s = 0;
    }
    else
    {
        s = diff / (1 - abs(2*l-1));
    }
    
        return float3(h, s, l);
}

//function based on https://www.rapidtables.com/convert/color/hsl-to-rgb.html
float3 HSLToRGB2(float3 hsl)
{
    float C = (1 - abs(2*hsl.b -1)) * hsl.g;
    float X = C * (1 - abs(fmod((hsl.r / 60), 2) - 1));
    float m = hsl.b - C/2;
    
    float3 rgb;
    
    if (hsl.r >= 0 && hsl.r < 60)
    {
        rgb = float3(C, X, 0);
    }
    else if (hsl.r >= 60 && hsl.r < 120)
    {
        rgb = float3(X, C, 0);
    }
    else if (hsl.r >= 120 && hsl.r < 180)
    {
        rgb = float3(0, C, X);
    }
    else if (hsl.r >= 180 && hsl.r < 240)
    {
        rgb = float3(0, X, C);
    }
    else if (hsl.r >= 240 && hsl.r < 300)
    {
        rgb = float3(X, 0, C);
    }
    else if (hsl.r >= 300 && hsl.r < 360)
    {
        rgb = float3(C, 0, X);
    }
    
    rgb += m;
    
    return rgb;
}


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float strength = 3.0f;
	
	// Calculate alpha to display the effect in a softened circle, could use a texture rather than calculations for the same task.
	// Uses the second set of area texture coordinates, which range from (0,0) to (1,1) over the area being processed
    float softEdge = 0.20f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge); // Soft circle calculation based on fact that this circle has a radius of 0.5 (as area UVs go from 0->1)
    
 
    //convert to HSL1
    float3 HSLColour = RGBToHSL2(gTintColour);
    
    //changing hue
    HSLColour.r += 20*gFrameTime;
    
   while (HSLColour.r >= 360)
    {
        HSLColour.r -= 360;
    }
    
    float3 RGBColour = HSLToRGB2(HSLColour);
    
    
    //convert to HSL2
    float3 HSLColour2 = RGBToHSL2(gTintColour2);
    
    //changing hue
    HSLColour2.r += 20 * gFrameTime;
    
    while (HSLColour2.r >= 360)
    {
        HSLColour2.r -= 360;
    }

    float3 RGBColour2 = HSLToRGB2(HSLColour2);
    
    //interpolating between the two colours 
    float3 finalTint = lerp(RGBColour, RGBColour2, input.sceneUV.y) * strength;
    // Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)
    float3 colour = SceneTexture.Sample(PointSample, input.sceneUV).rgb * finalTint;
   
	// Got the RGB from the scene texture, and calculated alpha
    return float4(colour, 1.0f);
}