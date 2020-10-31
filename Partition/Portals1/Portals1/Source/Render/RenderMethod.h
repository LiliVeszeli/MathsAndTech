/*******************************************
	
	Shader.h

	Shader Support Declarations

********************************************/

#pragma once

#include <string>
using namespace std;
#include <d3dx9.h>

#include "Defines.h"
#include "CMatrix4x4.h"
#include "Camera.h"
#include "Light.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Render method types
//-----------------------------------------------------------------------------

// Customisable list of render methods available for use in materials and implemented in
// RenderMethod.cpp/.h. This list can be changed to to support new rendering methods
enum ERenderMethod
{
	PlainColour        = 0,
	PlainTexture       = 1,
	VertexLit          = 2,
	VertexLitTex       = 3,
	NumRenderMethods // Leave this entry at end
};

// Pointer to a function to initialise a shader for rendering
typedef void (*PShaderFn)(int method, CMatrix4x4* worldMatrix, CCamera* camera);

// Structure defining a rendering method - defines vertex and pixel shader source files and
// initialisation functions as well as the DirectX pointers associated with the shaders
struct SRenderMethod
{
	string    vertexShaderFile;
	PShaderFn vertexShaderFn;
	string    pixelShaderFile;
	PShaderFn pixelShaderFn;

	LPDIRECT3DVERTEXSHADER9 vertexShader;
	LPD3DXCONSTANTTABLE     vertexConsts;
	LPDIRECT3DPIXELSHADER9  pixelShader;
	LPD3DXCONSTANTTABLE     pixelConsts;
};

// A vertex element structure and DirectX vertex declaration for a render method
struct RenderMethodDecl
{
	LPD3DVERTEXELEMENT9          vertexElts;
	LPDIRECT3DVERTEXDECLARATION9 vertexDecl;
};


//-----------------------------------------------------------------------------
// Method constants / related functions
//-----------------------------------------------------------------------------

// Set the material colour and specular power used in all methods
void SetMaterialColour( const D3DXCOLOR& diffuseColour, float specularPower );

// Set the ambient light colour used for all methods
void SetAmbientLight( const SColourRGBA& colour );

// Set the light list to use for all methods
void SetLights( CLight** lights );


//-----------------------------------------------------------------------------
// Method usage
//-----------------------------------------------------------------------------

// Use the given method for rendering, pass the world matrix and camera to be used for the shaders
void UseMethod( int method, CMatrix4x4* worldMatrix, CCamera* camera );


//-----------------------------------------------------------------------------
// Method initialisation
//-----------------------------------------------------------------------------

// Initialises the given render method (vertex + pixel shader), returns true on success
bool LoadMethod( int method );

// Releases the DirectX data associated with all render methods
void ReleaseMethods();


//-----------------------------------------------------------------------------
// Shader initialisation
//-----------------------------------------------------------------------------

// Flags used when creating shaders - change value to enable shader debugging
const DWORD SHADER_FLAGS = 0;
//const DWORD SHADER_FLAGS = D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;

// Load and compiler a HLSL vertex shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadVertexShader( const string& fileName, LPDIRECT3DVERTEXSHADER9* vertexShader,
					   LPD3DXCONSTANTTABLE* constants );

// Load and compiler a HLSL pixel shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadPixelShader( const string& fileName, LPDIRECT3DPIXELSHADER9* pixelShader,
					  LPD3DXCONSTANTTABLE* constants );


} // namespace gen