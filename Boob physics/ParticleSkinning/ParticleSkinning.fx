//--------------------------------------------------------------------------------------
// File: ParticleSkinning.fx
//
// Soft Body Skinning
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------

//****|SOFTBODY|*************************************************************//

// Array of matrices for particles in the system, used as "bones" for skinning
static const int MAX_PARTICLES = 80;
float4x4 ParticleMatrices[MAX_PARTICLES];

//***************************************************************************//

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;
float4x4 ViewProjMatrix;

// Information used for lighting (in the vertex or pixel shader)
float3 Light1Pos;
float3 Light2Pos;
float3 Light1Colour;
float3 Light2Colour;
float3 AmbientColour;
float  SpecularPower;
float3 CameraPos;

// Variable used to tint each light model to show the colour that it emits
float3 TintColour;

// Diffuse texture map
Texture2D DiffuseMap;

// Samplers to use with the above textures
SamplerState TrilinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// This structure describes the vertex data to be sent into the vertex shader
struct VS_INPUT
{
    float3 Pos    : POSITION;
    float3 Normal : NORMAL;
	float2 UV     : TEXCOORD0;
};

// This structure describes the vertex data to be sent into the vertex shader
struct VS_SKINNING_INPUT
{
    float3 Pos        : POSITION;
	float4 Weights    : BLENDWEIGHTS; // Weights for up to 4 particles affecting this vertex, sum must be <= 1.0 (see comment below)
	uint4  Influences : BLENDINDICES; // Which particles that influence this vertex (indexes into ParticleOffsets array above)
    float3 Normal     : NORMAL;
	float2 UV         : TEXCOORD0;
};


// The input required for the per-pixel lighting pixel shader, containing a 2D position, lighting colours and texture coordinates
struct PS_LIGHTING_INPUT
{
    float4 ProjPos     : SV_Position;
	float3 WorldPos    : POSITION;
	float3 WorldNormal : NORMAL;
    float2 UV          : TEXCOORD0;
};


// More basic techniques don't deal with lighting
struct PS_BASIC_INPUT
{
    float4 ProjPos : SV_Position;
    float2 UV      : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

// This vertex shader passes on the vertex position and normal to the pixel shader for per-pixel lighting
PS_LIGHTING_INPUT VS_ParticleSkinning( VS_SKINNING_INPUT vIn )
{
	PS_LIGHTING_INPUT vOut;

	///////////////////////////
	// Matrix transformations

	// Add 4th element to model vertex position / normal to prepare for matrix multiplication
	float4 modelPos    = float4(vIn.Pos,    1.0f);
	float4 modelNormal = float4(vIn.Normal, 0.0f);

	// In standard skinning, all vertices are affected by at least one bone. In this system, some model vertices
	// may only be partly affected by particles, some not at all. So the sum of the particle influences for a 
	// particular vertex may be < 1, the remainder being how much that vertex is just affected by the model.
	float modelWeight = 1.0f - vIn.Weights[0] - vIn.Weights[1] - vIn.Weights[2] - vIn.Weights[3];

	//****|SOFTBODY| Combine influences from the model and from up to 4 influencing particles to get skinned result ****//
	float4 worldPos  = mul( modelPos, WorldMatrix )                         * modelWeight; 
	worldPos        += mul( modelPos, ParticleMatrices[vIn.Influences[0]] ) * vIn.Weights[0]; 
	worldPos        += mul( modelPos, ParticleMatrices[vIn.Influences[1]] ) * vIn.Weights[1]; 
	worldPos        += mul( modelPos, ParticleMatrices[vIn.Influences[2]] ) * vIn.Weights[2]; 
	worldPos        += mul( modelPos, ParticleMatrices[vIn.Influences[3]] ) * vIn.Weights[3]; 

	// Similar process for normals
	float4 worldNormal  = mul( modelNormal, WorldMatrix )                         * modelWeight; 
	worldNormal        += mul( modelNormal, ParticleMatrices[vIn.Influences[0]] ) * vIn.Weights[0]; 
	worldNormal        += mul( modelNormal, ParticleMatrices[vIn.Influences[1]] ) * vIn.Weights[1]; 
	worldNormal        += mul( modelNormal, ParticleMatrices[vIn.Influences[2]] ) * vIn.Weights[2]; 
	worldNormal        += mul( modelNormal, ParticleMatrices[vIn.Influences[3]] ) * vIn.Weights[3]; 


	///////////////////////////
	// Output for Pixel Shader

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );

	// For pixel lighting, pass the world position & world normal to the pixel shader
	vOut.WorldPos    = worldPos;
	vOut.WorldNormal = worldNormal;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;

	return vOut;
}


// This vertex shader passes on the vertex position and normal to the pixel shader for per-pixel lighting
PS_LIGHTING_INPUT VS_PixelLitTex( VS_INPUT vIn )
{
	PS_LIGHTING_INPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );

	// Transform the vertex normal from model space into world space (almost same as first lines of code above)
	float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
	vOut.WorldNormal = mul( modelNormal, WorldMatrix ).xyz;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;

	return vOut;
}


// Basic vertex shader to transform 3D model vertices to 2D and pass UVs to the pixel shader
PS_BASIC_INPUT VS_BasicTransformTex( VS_INPUT vIn )
{
	PS_BASIC_INPUT vOut;
	
	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	
	// Pass texture coordinates (UVs) on to the pixel shader
	vOut.UV = vIn.UV;

	return vOut;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------

// Pixel shader that calculates per-pixel lighting and combines with diffuse and specular map
//
float4 PS_PixelLitDiffuseMap( PS_LIGHTING_INPUT pIn ) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	float3 worldNormal = normalize(pIn.WorldNormal); 


	///////////////////////
	// Calculate lighting

	// Calculate direction of camera
	float3 CameraDir = normalize(CameraPos - pIn.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)
	
	//// LIGHT 1
	float3 Light1Dir = normalize(Light1Pos - pIn.WorldPos.xyz);   // Direction for each light is different
	float3 Light1Dist = length(Light1Pos - pIn.WorldPos.xyz); 
	float3 DiffuseLight1 = Light1Colour * max( dot(worldNormal.xyz, Light1Dir), 0 ) / Light1Dist;
	float3 halfway = normalize(Light1Dir + CameraDir);
	float3 SpecularLight1 = DiffuseLight1 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );

	//// LIGHT 2
	float3 Light2Dir = normalize(Light2Pos - pIn.WorldPos.xyz);
	float3 Light2Dist = length(Light2Pos - pIn.WorldPos.xyz);
	float3 DiffuseLight2 = Light2Colour * max( dot(worldNormal.xyz, Light2Dir), 0 ) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 SpecularLight2 = DiffuseLight2 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2;
	float3 SpecularLight = SpecularLight1 + SpecularLight2;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture
	float4 DiffuseMaterial = DiffuseMap.Sample( TrilinearWrap, pIn.UV );
	
	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
	float3 SpecularMaterial = DiffuseMaterial.a;

	
	////////////////////
	// Combine colours 
	
	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial.rgb * DiffuseLight + SpecularMaterial * SpecularLight;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}


// A pixel shader that just tints a (diffuse) texture with a fixed colour
//
float4 PS_TintDiffuseMap( PS_BASIC_INPUT pIn ) : SV_Target
{
	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMapColour = DiffuseMap.Sample( TrilinearWrap, pIn.UV );

	// Tint by global colour (set from C++)
	diffuseMapColour.rgb *= TintColour / 10;

	return diffuseMapColour;
}



//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

// States are needed to switch between additive blending for the lights and no blending for other models

RasterizerState CullNone  // Cull none of the polygons, i.e. show both sides
{
	CullMode = None;
};
RasterizerState CullBack  // Cull back side of polygon - normal behaviour, only show front of polygons
{
	CullMode = Back;
};


DepthStencilState DepthWritesOff // Don't write to the depth buffer - polygons rendered will not obscure other polygons
{
	DepthFunc      = LESS;
	DepthWriteMask = ZERO;
};
DepthStencilState DepthWritesOn  // Write to the depth buffer - normal behaviour 
{
	DepthFunc      = LESS;
	DepthWriteMask = ALL;
};
DepthStencilState DisableDepth   // Disable depth buffer entirely
{
	DepthFunc      = ALWAYS;
	DepthWriteMask = ZERO;
};


BlendState NoBlending // Switch off blending - pixels will be opaque
{
    BlendEnable[0] = FALSE;
};
BlendState AdditiveBlending // Additive blending is used for lighting effects
{
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
};


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

//****|SOFTBODY|*************************************************************//

// Particle skinning - vertex shader uses nearby particles to perform skinning, pixel shader is ordinary pixel-lighting
technique10 ParticleSkinning
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS_ParticleSkinning() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, PS_PixelLitDiffuseMap() ) );

		// Switch off blending states
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullBack ); 
		SetDepthStencilState( DepthWritesOn, 0 );
	}
}


//**********************************************************************//


// Per-pixel lighting with diffuse map
technique10 PixelLitTex
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS_PixelLitTex() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, PS_PixelLitDiffuseMap() ) );

		// Switch off blending states
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullBack ); 
		SetDepthStencilState( DepthWritesOn, 0 );
	}
}


// Additive blended texture. No lighting, but uses a global colour tint. Used for light models
technique10 AdditiveTexTint
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS_BasicTransformTex() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, PS_TintDiffuseMap() ) );

		SetBlendState( AdditiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullNone ); 
		SetDepthStencilState( DepthWritesOff, 0 );
     }
}
