/*******************************************
	
	RenderMethod.cpp

	A set of render methods - different
	vertex/pixel shader combinations that
	can be selected for different parts of
	a mesh being rendered

********************************************/

#include "RenderMethod.h"
#include "MathDX.h"

namespace gen
{

// Get reference to global DirectX render device from another source file
// Not good practice - these functions should be part of a class with this as a member
extern LPDIRECT3DDEVICE9 g_pd3dDevice;


//-----------------------------------------------------------------------------
// Method constants / related functions
//-----------------------------------------------------------------------------

// Location of shaders
static const string ShaderFolder = "Source\\Render\\";


// Material colour and specular power used for all methods, altered through function below
static D3DXCOLOR m_DiffuseColour( 1.0f, 1.0f, 1.0f, 1.0f );
static float m_SpecularPower = 0.0f;

// Ambient colour used for all methods, can be altered through function below
static SColourRGBA m_AmbientLight( 0.0f, 0.0f, 0.0f, 0.0f );

// Pointer to light list used for all methods, can be altered through function below
static CLight** m_Lights = 0;


// Set the material colour and specular power used in all methods
void SetMaterialColour( const D3DXCOLOR& diffuseColour, float specularPower )
{
	m_DiffuseColour = diffuseColour;
	m_SpecularPower = specularPower;
}

// Set the ambient light colour used for all methods
void SetAmbientLight( const SColourRGBA& colour )
{
	m_AmbientLight = colour;
}

// Set the light list to use for all methods
void SetLights( CLight** lights )
{
	m_Lights = lights;
}


//-----------------------------------------------------------------------------
// Currently available render methods (shaders)
//-----------------------------------------------------------------------------

// The list of render method names is ERenderMethod in RenderMethod.h

// Prototypes for functions in array below
void VS_SingleXFormFn( int method, CMatrix4x4* worldMatrix, CCamera* camera );
void VS_VertLit1Fn( int method, CMatrix4x4* worldMatrix, CCamera* camera );
void PS_NullFn( int method, CMatrix4x4* worldMatrix, CCamera* camera );
void PS_PlainColourFn( int method, CMatrix4x4* worldMatrix, CCamera* camera );
void PS_FogFn( int method, CMatrix4x4* worldMatrix, CCamera* camera );

// Array of data defining each render method in turn. Each method hass a vertex and pixel shader
// source file and a function to initialise them for rendering. The DirectX shader pointers are
// also provided (initially 0). The functions are defined using a function pointer type (PShaderFn
// in RenderMethod.h) - these functions must all have the same style of prototype as shown above
SRenderMethod renderMethods[NumRenderMethods] =
{
	"XFormOnly.vsh",      VS_SingleXFormFn,    "PlainColour.psh",        PS_PlainColourFn,    0, 0, 0, 0,
	"XFormTex.vsh",       VS_SingleXFormFn,    "TextureOnly.psh",        PS_NullFn,           0, 0, 0, 0,
	"VertexLit1.vsh",     VS_VertLit1Fn,       "DiffuseColour.psh",      PS_FogFn,            0, 0, 0, 0,
	"VertexLit1Tex.vsh",  VS_VertLit1Fn,       "DiffuseColourTex.psh",   PS_FogFn,            0, 0, 0, 0,
	"XFormOnly.vsh",      VS_SingleXFormFn,    "ClearDepth.psh",         PS_PlainColourFn,    0, 0, 0, 0,
};


//-----------------------------------------------------------------------------
// Vertex data needed for render methods
//-----------------------------------------------------------------------------

// Each render methods needs a definition of its source vertex elements, this is called the
// vertex declaration in DirectX

// Standard vertex elements: vertex coordinate + normal + texture coords(UV)
D3DVERTEXELEMENT9 PosNormUV[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END() // Terminate a vertex declaration with special element
};

// Vertex coordinate and normal
D3DVERTEXELEMENT9 PosNorm[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	D3DDECL_END() // Terminate a vertex declaration with special element
};

// Vertex coordinate and texture coords
D3DVERTEXELEMENT9 PosUV[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END() // Terminate a vertex declaration with special element
};

// Vertex coordinate only
D3DVERTEXELEMENT9 PosOnly[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	D3DDECL_END() // Terminate a vertex declaration with special element
};

// Vertex element structures and DX vertex declarations (initially 0) for each render method.
// All current shaders use the same vertex declaration. Would need to add more for more complex
// shaders (e.g. normal mapping would need to include a vertex tangent)
RenderMethodDecl renderMethodDecls[NumRenderMethods] = 
{
	PosOnly,   0,
	PosUV,     0,
	PosNorm,   0,
	PosNormUV, 0,
	PosOnly,   0,
};


//-----------------------------------------------------------------------------
// Method usage
//-----------------------------------------------------------------------------

// Use the given method for rendering, pass the world matrix and camera to be used for the shaders
void UseMethod( int method, CMatrix4x4* worldMatrix, CCamera* camera )
{
	// Set shaders in DirectX
	g_pd3dDevice->SetVertexDeclaration( renderMethodDecls[method].vertexDecl );
	g_pd3dDevice->SetVertexShader( renderMethods[method].vertexShader );
	g_pd3dDevice->SetPixelShader( renderMethods[method].pixelShader );

	// Initialise shader constants and other render settings
	renderMethods[method].vertexShaderFn( method, worldMatrix, camera );
	renderMethods[method].pixelShaderFn( method, worldMatrix, camera );
}


//-----------------------------------------------------------------------------
// Method initialisation
//-----------------------------------------------------------------------------

// Initialises the given render method (vertex + pixel shader), returns true on success
bool LoadMethod( int method )
{
	// If the vertex shader for this method has not already been initialised
	if (!renderMethods[method].vertexShader)
	{
		// Load the vertex shader file specified above, storing resultant DirectX data
		if (!LoadVertexShader( renderMethods[method].vertexShaderFile,
                               &renderMethods[method].vertexShader,
		                       &renderMethods[method].vertexConsts ))
		{
			return false;
		}
	}
	if (!renderMethods[method].pixelShader)
	{
		// Load the vertex shader file specified above, storing resultant DirectX data
		if (!LoadPixelShader( renderMethods[method].pixelShaderFile,
		                      &renderMethods[method].pixelShader,
		                      &renderMethods[method].pixelConsts ))
		{
			return false;
		}
	}

	if (!renderMethodDecls[method].vertexDecl)
	{
		if (FAILED(g_pd3dDevice->CreateVertexDeclaration( renderMethodDecls[method].vertexElts,
		                                                  &renderMethodDecls[method].vertexDecl )))
		{
			return false;
		}
	}

	return true;
}

// Releases the DirectX data associated with all render methods
void ReleaseMethods()
{
	for (int method = 0; method < NumRenderMethods; ++method)
	{
		if (renderMethodDecls[method].vertexDecl)
		{
			renderMethodDecls[method].vertexDecl->Release();
		}
		if (renderMethods[method].pixelConsts)
		{
			renderMethods[method].pixelConsts->Release();
		}
		if (renderMethods[method].pixelShader)
		{
			renderMethods[method].pixelShader->Release();
		}
		if (renderMethods[method].vertexConsts)
		{
			renderMethods[method].vertexConsts->Release();
		}
		if (renderMethods[method].vertexShader)
		{
			renderMethods[method].vertexShader->Release();
		}
	}
}


//-----------------------------------------------------------------------------
// Shader initialisation
//-----------------------------------------------------------------------------

// Load and compiler a HLSL vertex shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadVertexShader( const string& fileName, LPDIRECT3DVERTEXSHADER9* vertexShader,
					   LPD3DXCONSTANTTABLE* constants )
{
	// Temporary variable to hold compiled pixel shader code
    LPD3DXBUFFER pShaderCode;

	// Compile external HLSL pixel shader into shader code to submit to the hardware
	string fullFileName = ShaderFolder + fileName;
	HRESULT hr = 
		D3DXCompileShaderFromFile( fullFileName.c_str(),// File containing pixel shader (HLSL)
			                       NULL, NULL,       // Advanced compilation options - not needed here
								   "main",           // Name of main function in the shader
								   "vs_2_0",         // Target vertex shader hardware - vs_1_1 is lowest level
												     // and will work on all video cards with a pixel shader
								   SHADER_FLAGS,     // Additional compilation flags (such as debug flags)
								   &pShaderCode,     // Ptr to variable to hold compiled shader code
								   NULL,             // Ptr to variable to hold error messages (not needed)
								   constants );      // Ptr to variable to hold constants for the shader
    if (FAILED(hr))
	{
		// Return if compilation failed
		return false;
	}

	// Create the pixel shader using the compiled shader code
    hr = g_pd3dDevice->CreateVertexShader( (DWORD*)pShaderCode->GetBufferPointer(), vertexShader );
    
	// Discard the shader code now the shader has been created 
	pShaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
    if (FAILED(hr))
	{
		return false;
	}

	return true;
}


// Load and compiler a HLSL pixel shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadPixelShader( const string& fileName, LPDIRECT3DPIXELSHADER9* pixelShader,
					  LPD3DXCONSTANTTABLE* constants )
{
	// Temporary variable to hold compiled pixel shader code
    LPD3DXBUFFER pShaderCode;

	// Compile external HLSL pixel shader into shader code to submit to the hardware
	string fullFileName = ShaderFolder + fileName;
	HRESULT hr = 
		D3DXCompileShaderFromFile( fullFileName.c_str(), // File containing pixel shader (HLSL)
			                       NULL, NULL,       // Advanced compilation options - not needed here
								   "main",           // Name of main function in the shader
								   "ps_2_0",         // Target pixel shader hardware - ps_1_1 is lowest level
												     // and will work on all video cards with a pixel shader
								   SHADER_FLAGS,     // Additional compilation flags (such as debug flags)
								   &pShaderCode,     // Ptr to variable to hold compiled shader code
								   NULL,             // Ptr to variable to hold error messages (not needed)
								   constants );      // Ptr to variable to hold constants for the shader
    if (FAILED(hr))
	{
		// Return if compilation failed
		return false;
	}

	// Create the pixel shader using the compiled shader code
    hr = g_pd3dDevice->CreatePixelShader( (DWORD*)pShaderCode->GetBufferPointer(), pixelShader );
    
	// Discard the shader code now the shader has been created 
	pShaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
    if (FAILED(hr))
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Vertex shader initialisation functions
//-----------------------------------------------------------------------------

// Each of these functions sets up shader constants and other render settings for their render
// method. They are passed the render method index, world matrix of the model, and camera

// Pass single combined world/view/projection matrix to the vertex shader
void VS_SingleXFormFn( int method, CMatrix4x4* worldMatrix, CCamera* camera )
{
	LPD3DXCONSTANTTABLE shaderConsts = renderMethods[method].vertexConsts;

	D3DXMATRIXA16 matWorldViewProj = ToD3DXMATRIX( *worldMatrix * camera->GetViewProjMatrix() );
	shaderConsts->SetMatrix( g_pd3dDevice, "WorldViewProjMatrix", &matWorldViewProj );
}

// Pass data to vertex shaders that perform vertex lighting (1 point light)
// Passes full range of data - some shaders don't need all of it. This
// reduces the number of these functions at the expense of redundancy
void VS_VertLit1Fn( int method, CMatrix4x4* worldMatrix, CCamera* camera )
{
	LPD3DXCONSTANTTABLE shaderConsts = renderMethods[method].vertexConsts;

	D3DXMATRIXA16 matViewProj = ToD3DXMATRIX( camera->GetViewProjMatrix() );
	shaderConsts->SetMatrix( g_pd3dDevice, "ViewProjMatrix", &matViewProj );

	D3DXMATRIX* matWorld = ToD3DXMATRIXPtr( worldMatrix );
	shaderConsts->SetMatrix( g_pd3dDevice, "WorldMatrix", matWorld );

	D3DXVECTOR3 cameraPos = ToD3DXVECTOR( camera->Position() );
	shaderConsts->SetFloatArray( g_pd3dDevice, "CameraPosition", (FLOAT*)&cameraPos, 3 ); // If needed

	shaderConsts->SetFloatArray( g_pd3dDevice, "AmbientLight", (FLOAT*)&m_AmbientLight, 3 );
	shaderConsts->SetFloatArray( g_pd3dDevice, "LightPosition", (FLOAT*)&m_Lights[0]->GetPosition(), 3 );
	shaderConsts->SetFloatArray( g_pd3dDevice, "LightColour", (FLOAT*)&m_Lights[0]->GetColour(), 3 );
	shaderConsts->SetFloat( g_pd3dDevice, "LightBrightness", m_Lights[0]->GetBrightness() );

	shaderConsts->SetFloat( g_pd3dDevice, "SpecularPower", m_SpecularPower ); // If needed
	shaderConsts->SetFloatArray( g_pd3dDevice, "MaterialColour", (FLOAT*)&m_DiffuseColour, 3 ); // If needed
}


//-----------------------------------------------------------------------------
// Pixel shader initialisation functions
//-----------------------------------------------------------------------------

// Function for pixel shaders that don't require any setup
void PS_NullFn( int method, CMatrix4x4* worldMatrix, CCamera* camera )
{
	// Nothing to do, just switch off states from other functions
	g_pd3dDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
}

// Function for pixel shader that just needs material colour
void PS_PlainColourFn( int method, CMatrix4x4* worldMatrix, CCamera* camera )
{
	// Switch off unneeded states
	g_pd3dDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );

	LPD3DXCONSTANTTABLE shaderConsts = renderMethods[method].pixelConsts;

	shaderConsts->SetFloatArray( g_pd3dDevice, "MaterialColour", (FLOAT*)&m_DiffuseColour, 3 );
}

// Function for pixel shaders that don't require special setup but adding fog
void PS_FogFn( int method, CMatrix4x4* worldMatrix, CCamera* camera )
{
	float start = 20.0f;
	float end = 900.0f;
	g_pd3dDevice->SetRenderState( D3DRS_FOGENABLE, TRUE );
	g_pd3dDevice->SetRenderState( D3DRS_FOGCOLOR, 0xE3DEB5 );
	g_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_LINEAR );
	g_pd3dDevice->SetRenderState( D3DRS_FOGSTART, *(DWORD*)&start );
	g_pd3dDevice->SetRenderState( D3DRS_FOGEND, *(DWORD*)&end );
}


} // namespace gen