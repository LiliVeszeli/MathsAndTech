/**********************************************
	VertexLit1.vsh

	Vertex shader that transforms the vertex
	position into 2D viewport space. Also
	calculates lighting for each vertex and
	passes diffuse colour to pixel shader 

	N.B. One point light with specular
***********************************************/

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// World matrix, combined view / projection matrix and camera position for transformation
// and lighting calculations
float4x3 WorldMatrix    : WORLD;
float4x4 ViewProjMatrix : VIEWPROJECTION;

// Current lighting information - ambient + one point light
float3 AmbientLight;
float3 LightPosition;
float3 LightColour;
float  LightBrightness;

// Shininess of material and camera position needed for specular calculation
float SpecularPower;
float3 CameraPosition;

// Material colour
float3 MaterialColour;


//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Input to Vertex Shader
struct VS_Input
{
	float3 Position  : POSITION;  // The position of the vertex in model space
	float3 Normal    : NORMAL;
};

// Output from Vertex Shader
struct VS_Output
{
	float4 Position      : POSITION;
	float3 DiffuseColour : COLOR0;
};

//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function. Calculates ambient and diffuse lighting from a single
// light and passes colour along with texture coordinate to pixel shader
void main( in VS_Input i, out VS_Output o ) 
{
    // Transform model vertex position to world space, then to viewport space
    float3 WorldPosition = mul( float4(i.Position, 1.0f), WorldMatrix );         
    o.Position = mul( float4(WorldPosition, 1.0f), ViewProjMatrix );

    // Transform model normal to world space
    float3 WorldNormal = normalize( mul( i.Normal, (float3x3)WorldMatrix ) );
	
	
	//**********************
	// Lighting preparation

	// Get normalised vector to camera for specular equation (common for all lights)
	float3 CameraDir = normalize( CameraPosition - WorldPosition );


	//******************
	// Lighting equation

	// Calculate diffuse lighting from the 1st light. Standard equation: Diffuse = max(0, N.L)
	float3 LightDir = LightPosition - WorldPosition;
	float LightDist = length( LightDir );
	float LightStrength = saturate( LightBrightness / LightDist );
	LightDir /= LightDist;
	float3 DiffuseColour = LightStrength * LightColour * saturate( dot( WorldNormal, LightDir ) );

	// Calculate specular lighting from the 1st light. Standard equation: Specular = max(0, (N.H)^p)
	// Slight tweak here: multiply by diffuse colour
	float3 Halfway = normalize( CameraDir + LightDir );
	float3 SpecularColour = DiffuseColour * saturate( pow( dot( WorldNormal, Halfway ), SpecularPower ) );


	//******************
	// Final blending

	// To get output colour, multiply material and diffuse colour, then add the specular
	o.DiffuseColour = (AmbientLight + DiffuseColour) * MaterialColour + SpecularColour; 
}
