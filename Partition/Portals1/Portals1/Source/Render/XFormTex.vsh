/**********************************************
	XFormTex.vsh

	Simple vertex shader that transforms the
	vertex position into 2D viewport space and
	passes a texture coordinate to pixel shader
***********************************************/

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Combined world / view / projection matrix
float4x4 WorldViewProjMatrix : WORLDVIEWPROJECTION;


//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Input to Vertex Shader
struct VS_Input
{
	float3 Position  : POSITION;
	float2 TexCoord0 : TEXCOORD0;
};

// Output from Vertex Shader
struct VS_Output
{
	float4 Position : POSITION;
	float2 TexCoord0: TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function
void main( in VS_Input i, out VS_Output o ) 
{
    // Transform model vertex position to viewport space, then output it
    o.Position = mul( float4(i.Position, 1.0f), WorldViewProjMatrix );
    
    // Copy texture coord
    o.TexCoord0 = i.TexCoord0;
}
