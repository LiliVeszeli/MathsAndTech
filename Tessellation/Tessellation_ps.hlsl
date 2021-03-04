//--------------------------------------------------------------------------------------
// Tessellation Pixel Shader
//--------------------------------------------------------------------------------------
// The pixel shader for displacement mapping is essentially the same as normal mapping. We have created extra tessellated geometry and displaced the positions
// to create a real bumpy surface. We used a height map to get the geometry displacements, so just use an associated normal map to get the normals (we can
// calculate the normals from the geometry, but that is time consuming and tricky in a shader - a normal map is much more effective here)

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to textures that has been loaded from the C++ side and stored in GPU memory.
Texture2D DiffuseSpecularMap : register(t0);
Texture2D NormalHeightMap    : register(t1); 
SamplerState TexSampler      : register(s0);; // A sampler is a filter for a texture like bilinear, trilinear or anisotropic - this is the sampler used for the texture above


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float4 main(NormalMappingPixelShaderInput input) : SV_Target
{
	//************************
	// Normal Map Extraction
	//************************

	// Normalize interpolated normals
	float3 worldNormal  = normalize( input.worldNormal );
	float3 worldTangent = normalize( input.worldTangent );

	// Calculate binormal and form a matrix for tangent space (can store the binormal in the source mesh data to avoid this calculation if we wish)
	float3 worldBitangent = cross(worldNormal, worldTangent);
	float3x3 invTangentMatrix = float3x3(worldTangent, worldBitangent, worldNormal);
	
	// Get the texture normal from the normal map. The r,g,b pixel values actually store x,y,z components of a normal. However, r,g,b
	// values are stored in the range 0->1, whereas the x, y & z components should be in the range -1->1. So some scaling is needed
	float3 textureNormal = 2.0f * NormalHeightMap.Sample(TexSampler, input.uv).rgb - 1.0f; // Scale from 0->1 to -1->1

	// Now convert the texture normal into world space using the inverse tangent matrix
	worldNormal = mul(textureNormal, invTangentMatrix);


	///////////////////////
	// Calculate lighting

    // Lighting equations
    float3 cameraDirection = normalize(gCameraPosition - input.worldPosition);

    // Light 1
    float3 light1Vector    = gLight1Position - input.worldPosition;
    float  light1Distance  = length(light1Vector);
    float3 light1Direction = light1Vector / light1Distance; // Quicker than normalising as we have length for attenuation
    float3 diffuseLight1   = gLight1Colour * max(dot(worldNormal, light1Direction), 0) / light1Distance;

    float3 halfway = normalize(light1Direction + cameraDirection);
    float3 specularLight1 =  diffuseLight1 * pow(max(dot(worldNormal, halfway), 0), gSpecularPower); 


    // Light 2
    float3 light2Vector    = gLight2Position - input.worldPosition;
    float  light2Distance  = length(light2Vector);
    float3 light2Direction = light2Vector / light2Distance;
    float3 diffuseLight2 = gLight2Colour * max(dot(worldNormal, light2Direction), 0) / light2Distance;

    halfway = normalize(light2Direction + cameraDirection);
    float3 specularLight2 =  diffuseLight2 * pow(max(dot(worldNormal, halfway), 0), gSpecularPower);

	// Sum the effect of the lights - add the ambient at this stage rather than for each light (or we will get too much ambient)
	float3 diffuseLight = gAmbientColour + diffuseLight1 + diffuseLight2;
	float3 specularLight = specularLight1 + specularLight2;
	

    // Sample diffuse material colour for this pixel from a texture using a given sampler that you set up in the C++ code
    // Ignoring any alpha in the texture, just reading RGB
    float4 textureColour = DiffuseSpecularMap.Sample(TexSampler, input.uv);
    float3 diffuseMaterialColour = textureColour.rgb;
    float specularMaterialColour = textureColour.a;

    // Combine lighting with texture colours
    float3 finalColour = diffuseLight * diffuseMaterialColour + specularLight * specularMaterialColour;

    return float4(finalColour, 1.0f); // Always use 1.0f for alpha - no alpha blending in this lab
}