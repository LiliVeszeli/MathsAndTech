//--------------------------------------------------------------------------------------
// Soft Particle Pixel Shader
//--------------------------------------------------------------------------------------
// Saples a diffuse texture map and adjusts depth buffer value to give fake depth to the particle

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to a texture that has been loaded from the C++ side and stored in GPU memory.
// Note that textures are often called maps (because texture mapping describes wrapping a texture round a mesh).
// Get used to people using the word "texture" and "map" interchangably.
Texture2D    DiffuseMap : register(t0); // A diffuse map is the main texture for a model.
                                        // The t0 indicates this texture is in slot 0 and the C++ code must load the texture into the this slot
SamplerState TexSampler : register(s0); // A sampler is a filter for a texture like bilinear, trilinear or anisotropic

// Access depth buffer as a texture map - see C++ side to see how this was set up
Texture2D    DepthMap          : register(t1);
SamplerState PointClampSampler : register(s1); // This sampler switches off filtering (e.g. bilinear, trilinear) when accessing depth buffer


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Pixel shader entry point - each shader has a "main" function
PixelColourAndDepth main(ParticlePixelShaderInput input)
{
	PixelColourAndDepth output;
                                      
    // Extract diffuse material colour for this pixel from a texture. Using alpha channel here so use float4
	float4 diffuseMapColour = DiffuseMap.Sample(TexSampler, input.uv);
    
	

    float4 depthAdjust = diffuseMapColour.a * (input.scale / 5);
    float pixelDepth = (input.projectedPosition.z * input.projectedPosition.w - depthAdjust.a) / (input.projectedPosition.w - depthAdjust.a);
	
    output.depth = pixelDepth;
	
    float2 viewportUV = float2(input.projectedPosition.x, input.projectedPosition.y);
    viewportUV.x /= gViewportWidth;
    viewportUV.y /= gViewportHeight;
    
    float viewportDepth = DepthMap.Sample(PointClampSampler, viewportUV);
    viewportDepth *= input.projectedPosition.w;
    pixelDepth *= input.projectedPosition.w;
    
    float depthDiff = viewportDepth - pixelDepth;
    
    if (depthDiff < 0)
    {
        discard;
    }
    
    float depthFade = saturate(depthDiff / 0.025f) * saturate(pixelDepth / 3.0f);
    
  
	// Combine texture alpha with particle alpha
        output.colour = diffuseMapColour;
	output.colour.a *= input.alpha * depthFade; // If you increase the number of particles then you might want to reduce the 1.0f here to make them more transparent

	return output;
}