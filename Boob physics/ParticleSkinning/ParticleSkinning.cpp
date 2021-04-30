//--------------------------------------------------------------------------------------
//	ParticleSkinning.cpp
//
//	Rendering fur using shells
//--------------------------------------------------------------------------------------

#include <string>
#include <list>
#include <fstream>
using namespace std;

// General definitions used across all the project source files
#include "Defines.h"

// Declarations for supporting source files
#include "Model.h"   // Model class - new, encapsulates working with vertex/index data and world matrix
#include "Camera.h"  // Camera class - new, encapsulates the camera's view and projection matrix
#include "CTimer.h"  // Timer class - not DirectX
#include "Input.h"   // Input functions - not DirectX

// Maths
#include "CMatrix4x4.h"
#include "MathIO.h"
using namespace gen;

// Particle physics
#include "Particle.h"
#include "Spring.h"

#include "Resource.h" // Resource file (used to add icon for application)


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

// Models and cameras encapsulated in simple classes
CModel* SoftModel;
CModel* Stars;
CModel* Crate;
CModel* Ground;
CCamera* MainCamera;

// Textures
ID3D10ShaderResourceView* SoftModelDiffuseMap = NULL;
ID3D10ShaderResourceView* StarsDiffuseMap = NULL;
ID3D10ShaderResourceView* CrateDiffuseMap = NULL;
ID3D10ShaderResourceView* GroundDiffuseMap = NULL;
ID3D10ShaderResourceView* LightDiffuseMap = NULL;


// Light data stored manually, a light class would be helpful - but it's an assignment task for the second years
D3DXVECTOR4 BackgroundColour = D3DXVECTOR4( 0.3f, 0.3f, 0.4f, 1.0f );
D3DXVECTOR3 AmbientColour    = D3DXVECTOR3( 0.4f, 0.4f, 0.5f );
D3DXVECTOR3 Light1Colour     = D3DXVECTOR3( 0.8f, 0.8f, 1.0f ) * 16;
D3DXVECTOR3 Light2Colour     = D3DXVECTOR3( 1.0f, 0.8f, 0.6f ) * 40;
float SpecularPower = 8.0f;

// Display models where the lights are. One of the lights will follow an orbit
CModel* Light1;
CModel* Light2;
const float LightOrbitRadius = 30.0f;
const float LightOrbitSpeed  = 0.6f;

// Note: There are move & rotation speed constants in Defines.h



//--------------------------------------------------------------------------------------
// Shader Variables
//--------------------------------------------------------------------------------------
// Variables to connect C++ code to HLSL shaders

// Effects / techniques
ID3D10Effect*          Effect = NULL;
ID3D10EffectTechnique* PixelLitTexTechnique = NULL;
ID3D10EffectTechnique* AdditiveTexTintTechnique = NULL;
ID3D10EffectTechnique* ParticleSkinningTechnique = NULL;

// Matrices
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ParticleMatricesVar = NULL;  // Points to an array of matrices for particles in the shader

// Textures
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;

// Light Effect variables
ID3D10EffectVectorVariable* CameraPosVar = NULL;
ID3D10EffectVectorVariable* Light1PosVar = NULL;
ID3D10EffectVectorVariable* Light1ColourVar = NULL;
ID3D10EffectVectorVariable* Light2PosVar = NULL;
ID3D10EffectVectorVariable* Light2ColourVar = NULL;
ID3D10EffectVectorVariable* AmbientColourVar = NULL;
ID3D10EffectScalarVariable* SpecularPowerVar = NULL;

// Other 
ID3D10EffectVectorVariable* TintColourVar = NULL;


//--------------------------------------------------------------------------------------
// DirectX Variables
//--------------------------------------------------------------------------------------

// The main D3D interface
ID3D10Device* g_pd3dDevice = NULL;

// Variables used to setup D3D
IDXGISwapChain*         SwapChain = NULL;
ID3D10Texture2D*        DepthStencil = NULL;
ID3D10DepthStencilView* DepthStencilView = NULL;
ID3D10RenderTargetView* BackBufferRenderTarget = NULL;

// Variables used to setup the Window
HINSTANCE HInst = NULL;
HWND      HWnd = NULL;
int       g_ViewportWidth;
int       g_ViewportHeight;



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------

bool InitDevice();
void ReleaseResources();
bool LoadEffectFile();
bool InitScene();
void UpdateScene( float frameTime );
void RenderModels();
void RenderScene();
bool InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );


//*****|SOFTBODY|***************************************************************//
// Particle Physics Data
//******************************************************************************//
// Same spring & particle lists as the TL-Engine example. This application only manages a single
// particle-physics system, a more advanced app would encapsulate (and optimise) this data further

list<CParticle*> Particles;
list<CSpring*>   Springs;

const CVector3 GRAVITY = CVector3(0, -98.0f, 0);

// Only interested in unpinned particles for skinning (pinned particles just follow model so don't affect skinning)
// Store lists of their original (model-space) positions to generate vertex influences and weights for skinning. 
// Model vertices are affected by nearby particles, weighted by distance. The second list is the list of particle
// world matrices used for the skinning shader - these matrices operate just like bones in standard skinning
int NumSkinningParticles = 0;
CVector3* SkinningPositions = 0;
CMatrix4x4* SkinningMatrices = 0;


//****************************************************************************/
// Particle Physics Loading - same as TL-Engine version
//****************************************************************************/

// Get pointer to particle with given UID or NULL if no such particle
CParticle* ParticleFromUID( unsigned int UID )
{
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		if ((*itParticle)->GetUID() == UID) return (*itParticle);
		itParticle++;
	}
	return NULL;
}

// Remove all particles and springs
void ClearPhysicsSystem()
{
	while (Springs.size() > 0) 
	{
		delete (*Springs.begin());
		Springs.pop_front();
	}
	while (Particles.size() > 0)
	{
		delete (*Particles.begin());
		Particles.pop_front();
	}
}

// Load particle physics system from a file. Removes current system. Returns true on success
bool LoadParticlePhysics( string fileName )
{
	ifstream file( fileName.c_str(), ios::in );
	if (!file) return false;

	// Clear existing system
	ClearPhysicsSystem();

	// Get opening keyword
	string keyword;
	file >> keyword;
	if (keyword != "PARTICLES")
	{
		file.setstate( ios::failbit );
	}

	// Read each particle into temporary variables (to ensure parsing is correct), before creating actual particle
	CVector3 position;
	float mass;
	bool isPinned;
	unsigned int UID;
	while (file.good())
	{
		file >> keyword; // Each particle line must begin with "P" or have reached end of particle section
		if (!file.good() || keyword != "P") break;
		file >> position >> mass >> isPinned >> UID >> ws;  // ">> ws" skips any whitespace (moves to start of next line or eof)
		if (!file.fail())
		{
			CParticle* particle = new CParticle( position, mass, isPinned, UID );
			Particles.push_back( particle );
		}
	}

	if (keyword != "SPRINGS")
	{
		file.setstate( ios::failbit );
	}

	// Similar process with springs, but get particle pointers from their UIDs
	unsigned int particleUID1, particleUID2;
	float coefficient;
	float inertialLength;
	unsigned int springType; // Will be cast to CSpring::ESpringType
	while (file.good())
	{
		file >> keyword;
		if (keyword != "S") break;
		file >> particleUID1 >> particleUID2 >> coefficient >> inertialLength >> springType >> UID >> ws;
		if (!file.fail())
		{
			CParticle* particle1 = ParticleFromUID( particleUID1 );
			CParticle* particle2 = ParticleFromUID( particleUID2 );
			if (particle1 == NULL || particle2 == NULL)
			{
				file.setstate( ios::failbit );
			}
			else
			{
				CSpring* newSpring = new CSpring( particle1, particle2, coefficient, inertialLength,
				                                  static_cast<CSpring::ESpringType>(springType), UID );
				Springs.push_back( newSpring );
				particle1->AddSpring( newSpring );
				particle2->AddSpring( newSpring );
			}
		}
	}

	file.close();
	if (file.fail())
	{
		ClearPhysicsSystem();
		return false;
	}
	return true;
}


//****| INFO |****************************************************************//
// Particle Position Access
//****************************************************************************//

// Return number of particles used for skinning
int GetNumSkinningParticles()
{
	int numParticles = 0;
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		numParticles++;
		++itParticle;
	}
	return numParticles;
}

// Fill an array of CVector3 with the (model) position for each skinning particle. This is the original position as
// designed in the particle system editor. Used to calculate vertex weights for the skinning process. Vertices in the
// model are influenced by nearby particles, weighted by distance - same as standard skinning
void GetSkinningParticlePositions( CVector3* positions )
{
	int particle = 0;
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		positions[particle] = (*itParticle)->GetModelPosition();
		particle++;
		++itParticle;
	}
}

// Fill an array of CVector3 with the world matrices for each skinning particle. Can calculate a matrix for a particle, using the attached springs
// to help us choose local axes. However, these matrices, whilst consistent for the particle, may be oriented oddly and won't be appropriate to apply
// to model vertices. However, if we get the particle matrix in its initial position (relative to the model), and its matrix in at the current state
// in the simulation, then the transformation between these two matrices is the amount of movement & rotation applied to the particle relative to the
// model. I.e. this transform represents the world matrix for the particle and can be applied to model matrices. 
void GetSkinningParticleMatrices( CMatrix4x4* matrices, CMatrix4x4 worldMatrix )
{
	int particle = 0;
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		// Read comment at top. The transform between matrices A & B is equal to Inverse(A) * B. Do that with the particle matrices as described above
		// to give us a particle world matrix, usable for skinning
		matrices[particle] = Inverse( (*itParticle)->GetMatrix( CMatrix4x4::kIdentity, false ) ) * (*itParticle)->GetMatrix( worldMatrix, true );
		particle++;
		++itParticle;
	}
}


//****| INFO |****************************************************************//
// Particle Simulation Control
//****************************************************************************//

// Transform all particles in system by world matrix of given model, i.e. put particle system in same place as model
// Also if the model is scaled, then the intertial lengths of the springs, strings, etc. need to be scaled too
void InitPhysicsSystem( CModel* model, float scale )
{
	model->UpdateMatrix();
	D3DXMATRIX physicsMatrix = model->GetWorldMatrix();

	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		(*itParticle)->Initialise( CMatrix4x4(&physicsMatrix._11) );
		++itParticle;
	}

	list<CSpring*>::iterator itSpring = Springs.begin();
	while (itSpring != Springs.end())
	{
		(*itSpring)->SetInertialLength( (*itSpring)->GetInertialLength() * scale );
		++itSpring;
	}
}

// Transform particles in system to follow world matrix of given model. Only pinned particles will follow directly. Free particles
// will follow the model due to spring forces and constraints with the pinned particles. However, free particles do still keep track
// of their "pinned" position as the rendering needs to know the difference between their original and current position so it can
// flex the model vertices in the same way
void TransformPhysicsSystem( CModel* model )
{
	model->UpdateMatrix();
	D3DXMATRIX physicsMatrix = model->GetWorldMatrix();

	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		(*itParticle)->Transform( CMatrix4x4(&physicsMatrix._11)  );
		++itParticle;
	}
}

// Update the particle physics system - same as last week
void UpdatePhysicsSystem( float frameTime )
{
	// Update particle positions based on forces from springs and external forces (e.g. gravity)
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		CVector3 externalForces = (*itParticle)->GetMass() * GRAVITY;
		(*itParticle)->ApplyForces( frameTime, externalForces );
		++itParticle;
	}

	// Adjust springs (and their attached particles) based on any constraints (e.g. rods cannot change length)
	list<CSpring*>::iterator itSpring = Springs.begin();
	while (itSpring != Springs.end())
	{
		(*itSpring)->ApplyConstraints();
		++itSpring;
	}
}



//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
bool InitDevice()
{
	HRESULT hr = S_OK;


	////////////////////////////////
	// Initialise Direct3D

	// Calculate the visible area the window we are using - the "client rectangle" refered to in the first function is the 
	// size of the interior of the window, i.e. excluding the frame and title
	RECT rc;
	GetClientRect( HWnd, &rc );
	g_ViewportWidth = rc.right - rc.left;
	g_ViewportHeight = rc.bottom - rc.top;


	// Create a Direct3D device and create a swap-chain (create a back buffer to render to)
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_ViewportWidth;             // Target window size
	sd.BufferDesc.Height = g_ViewportHeight;           // --"--
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Pixel format of target window
	sd.BufferDesc.RefreshRate.Numerator = 60;          // Refresh rate of monitor
	sd.BufferDesc.RefreshRate.Denominator = 1;         // --"--
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.OutputWindow = HWnd;                            // Target window
	sd.Windowed = TRUE;                                // Whether to render in a window (TRUE) or go fullscreen (FALSE)
	hr = D3D10CreateDeviceAndSwapChain( NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_DEBUG, D3D10_SDK_VERSION, &sd, &SwapChain, &g_pd3dDevice );
	if( FAILED( hr ) ) return false;


	// Indicate that the back-buffer can be "viewed" as a render target - standard behaviour
	ID3D10Texture2D* pBackBuffer;
	hr = SwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );
	if( FAILED( hr ) ) return false;
	hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &BackBufferRenderTarget );
	pBackBuffer->Release();
	if( FAILED( hr ) ) return false;


	// Create a texture for a depth buffer
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = g_ViewportWidth;
	descDepth.Height = g_ViewportHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &DepthStencil );
	if( FAILED( hr ) ) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView( DepthStencil, &descDSV, &DepthStencilView );
	if( FAILED( hr ) ) return false;

	return true;
}


// Release the memory held by all objects created
void ReleaseResources()
{
	if( g_pd3dDevice ) g_pd3dDevice->ClearState();

	delete Light2;
	delete Light1;
	delete Ground;
	delete Crate;
	delete Stars;
	delete SoftModel;
	delete MainCamera;

	delete[] SkinningPositions;
	delete[] SkinningMatrices;

	if (GroundDiffuseMap)       GroundDiffuseMap->Release();
    if (LightDiffuseMap)        LightDiffuseMap->Release();
    if (CrateDiffuseMap)        CrateDiffuseMap->Release();
    if (StarsDiffuseMap)        StarsDiffuseMap->Release();
    if (SoftModelDiffuseMap)    SoftModelDiffuseMap->Release();
	if (Effect)                 Effect->Release();
	if (DepthStencilView)       DepthStencilView->Release();
	if (BackBufferRenderTarget) BackBufferRenderTarget->Release();
	if (DepthStencil)           DepthStencil->Release();
	if (SwapChain)              SwapChain->Release();
	if (g_pd3dDevice)           g_pd3dDevice->Release();
}



//--------------------------------------------------------------------------------------
// Load and compile Effect file (.fx file containing shaders)
//--------------------------------------------------------------------------------------

// All techniques in one file in this lab
bool LoadEffectFile()
{
	ID3D10Blob* pErrors; // This strangely typed variable collects any errors when compiling the effect file
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // These "flags" are used to set the compiler options

	// Load and compile the effect file
	HRESULT hr = D3DX10CreateEffectFromFile( L"ParticleSkinning.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &Effect, &pErrors, NULL );
	if( FAILED( hr ) )
	{
		if (pErrors != 0)  MessageBox( NULL, CA2CT(reinterpret_cast<char*>(pErrors->GetBufferPointer())), L"Error", MB_OK ); // Compiler error: display error message
		else               MessageBox( NULL, L"Error loading FX file. Ensure your FX file is in the same folder as this executable.", L"Error", MB_OK );  // No error message - probably file not found
		return false;
	}

	// Select techniques from the compiled effect file
	PixelLitTexTechnique      = Effect->GetTechniqueByName( "PixelLitTex" );
	AdditiveTexTintTechnique  = Effect->GetTechniqueByName( "AdditiveTexTint" );
	ParticleSkinningTechnique = Effect->GetTechniqueByName( "ParticleSkinning" );

	// Create variables to access global variables in the shaders from C++
	WorldMatrixVar      = Effect->GetVariableByName( "WorldMatrix"      )->AsMatrix();
	ViewMatrixVar       = Effect->GetVariableByName( "ViewMatrix"       )->AsMatrix();
	ProjMatrixVar       = Effect->GetVariableByName( "ProjMatrix"       )->AsMatrix();
	ViewProjMatrixVar   = Effect->GetVariableByName( "ViewProjMatrix"   )->AsMatrix();
	ParticleMatricesVar = Effect->GetVariableByName( "ParticleMatrices" )->AsMatrix();
	

	// Textures in shader (shader resources)
	DiffuseMapVar = Effect->GetVariableByName( "DiffuseMap" )->AsShaderResource();

	// Also access shader variables needed for lighting
	CameraPosVar     = Effect->GetVariableByName( "CameraPos"     )->AsVector();
	Light1PosVar     = Effect->GetVariableByName( "Light1Pos"     )->AsVector();
	Light1ColourVar  = Effect->GetVariableByName( "Light1Colour"  )->AsVector();
	Light2PosVar     = Effect->GetVariableByName( "Light2Pos"     )->AsVector();
	Light2ColourVar  = Effect->GetVariableByName( "Light2Colour"  )->AsVector();
	AmbientColourVar = Effect->GetVariableByName( "AmbientColour" )->AsVector();
	SpecularPowerVar = Effect->GetVariableByName( "SpecularPower" )->AsScalar();

	// Other shader variables
	TintColourVar = Effect->GetVariableByName( "TintColour"  )->AsVector();

	return true;
}



//--------------------------------------------------------------------------------------
// Scene Setup / Update
//--------------------------------------------------------------------------------------

// Create / load the camera, models and textures for the scene
bool InitScene()
{
	///////////////////
	// Create cameras

	MainCamera = new CCamera();
	MainCamera->SetPosition( D3DXVECTOR3(55, 35,-70) );
	MainCamera->SetRotation( D3DXVECTOR3(ToRadians(10.0f), ToRadians(-30.0f), 0.0f) ); // ToRadians is a new helper function to convert degrees to radians


	//****| INFO |*******************************************//
	// Load particle physics system from editor file
	// Load soft body model and attach physics system
	//*******************************************************//
	SoftModel = new CModel;

//	LoadParticlePhysics( "Rope.ptf" );
//	LoadParticlePhysics( "Woman.ptf" );

	NumSkinningParticles = GetNumSkinningParticles();
	SkinningPositions = new CVector3[NumSkinningParticles];
	SkinningMatrices = new CMatrix4x4[NumSkinningParticles];
	GetSkinningParticlePositions( SkinningPositions );

//	SoftModel->Load( "Rope.x", ParticleSkinningTechnique, false, NumSkinningParticles, SkinningPositions, 13.5f ); //*** The last parameter is the area of influence of each particle
//	SoftModel->SetPosition( D3DXVECTOR3(20, 10, 0) );
//	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"Rope.jpg", NULL, NULL, &SoftModelDiffuseMap,  NULL ) )) return false;

//	SoftModel->Load( "Woman1.x", ParticleSkinningTechnique, false, NumSkinningParticles, SkinningPositions, 2.0f ); //*** Worth tweaking the area of influence a little in this case
//	SoftModel->SetPosition( D3DXVECTOR3(20, 0, 0) );
//	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"WomanDiffuseSpecular.dds", NULL, NULL, &SoftModelDiffuseMap,  NULL ) )) return false;

	// Attach particle physics system to this model - pass scale of model (adjust spring inertial lengths to match)
	float Scale = 1.0f;
	SoftModel->SetScale( Scale );
	InitPhysicsSystem( SoftModel, Scale ); 

	//*******************************************************//


	// Create other models
	Stars     = new CModel;
	Crate     = new CModel;
	Ground    = new CModel;
	Light1    = new CModel;
	Light2    = new CModel;

	// Load .X files for each model
	if (!Stars-> Load( "Stars.x", PixelLitTexTechnique )) return false;
	if (!Crate-> Load( "CargoContainer.x", PixelLitTexTechnique )) return false;
	if (!Ground->Load( "Hills.x", PixelLitTexTechnique )) return false;
	if (!Light1->Load( "Light.x", AdditiveTexTintTechnique )) return false;
	if (!Light2->Load( "Light.x", AdditiveTexTintTechnique )) return false;

	// Initial positions
	Crate-> SetPosition( D3DXVECTOR3(-10, 0, 90) );
	Crate-> SetScale( 6.0f );
	Crate-> SetRotation( D3DXVECTOR3(0.0f, ToRadians(40.0f), 0.0f) );
	Stars-> SetScale( 10000.0f );
	Light1->SetPosition( D3DXVECTOR3(30, 10, 0) );
	Light1->SetScale( 4.0f );
	Light2->SetPosition( D3DXVECTOR3(100, 50, -50) );
	Light2->SetScale( 8.0f );


	//////////////////
	// Load textures

	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"CargoA.dds", NULL, NULL, &CrateDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StarsHi.jpg", NULL, NULL, &StarsDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"GrassDiffuseSpecular.dds", NULL, NULL, &GroundDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"flare.jpg", NULL, NULL, &LightDiffuseMap, NULL ) )) return false;

	return true;
}


// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (monoscopic version)
	MainCamera->Control( frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
	MainCamera->UpdateMatrices();
	
	// Move the soft body model
	const float SOFT_ROTATE_SPEED = 4.0f;
	const float SOFT_MOVE_SPEED = 20.0f;
	if (KeyHeld( Key_Comma ))  SoftModel->Rotate( D3DXVECTOR3(0, SOFT_ROTATE_SPEED * frameTime, 0) );
	if (KeyHeld( Key_Period )) SoftModel->Rotate(D3DXVECTOR3( 0, -SOFT_ROTATE_SPEED * frameTime, 0) );
	if (KeyHeld( Key_I )) SoftModel->Move( D3DXVECTOR3(0, SOFT_MOVE_SPEED * frameTime, 0) );
	if (KeyHeld( Key_K )) SoftModel->Move( D3DXVECTOR3(0, -SOFT_MOVE_SPEED * frameTime, 0) );
	if (KeyHeld( Key_L )) SoftModel->Move( D3DXVECTOR3(SOFT_MOVE_SPEED * frameTime, 0, 0) );
	if (KeyHeld( Key_J )) SoftModel->Move( D3DXVECTOR3(-SOFT_MOVE_SPEED * frameTime, 0, 0) );
	if (KeyHeld( Key_Y )) SoftModel->Move( D3DXVECTOR3(0, 0, SOFT_MOVE_SPEED * frameTime) );
	if (KeyHeld( Key_H )) SoftModel->Move( D3DXVECTOR3(0, 0, -SOFT_MOVE_SPEED * frameTime) );


	//****| INFO |**************************************************************************//
	// Update particle physics. Two steps:
	// - First update the particle positions to match the model's current position. Only 
	//   the pinned particles move directly, the free ones will follow due to spring forces
	// - Then update the particle physics system as normal
	//**************************************************************************************//
	TransformPhysicsSystem( SoftModel );
	UpdatePhysicsSystem( frameTime );

	// Update the orbiting light - a bit of a cheat with the static variable
	static float Rotate = 0.0f;
	Light1->SetPosition( SoftModel->GetPosition() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 10, sin(Rotate)*LightOrbitRadius) );
	Rotate -= LightOrbitSpeed * frameTime;
	Light1->UpdateMatrix();

	// Objects that don't move still need a world matrix - could do this in InitScene function (which occurs once at the start of the app)
	Stars->UpdateMatrix();
	Crate->UpdateMatrix();
	Ground->UpdateMatrix();
	Light2->UpdateMatrix();
}


//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render all models
void RenderModels()
{
	//****|SOFTBODY|*********************************/

	// Pass particle matrices to vertex shader
	GetSkinningParticleMatrices( SkinningMatrices, CMatrix4x4(SoftModel->GetWorldMatrix()) );
	ParticleMatricesVar->SetMatrixArray( (float*)SkinningMatrices, 0, NumSkinningParticles );

	// Set other shader variables, then render
	WorldMatrixVar->SetMatrix( (float*)SoftModel->GetWorldMatrix() );
    DiffuseMapVar->SetResource( SoftModelDiffuseMap );
	SoftModel->Render( ParticleSkinningTechnique );

	//************************************************/

	// Render other models in the scene
	WorldMatrixVar->SetMatrix( (float*)Crate->GetWorldMatrix() );
    DiffuseMapVar->SetResource( CrateDiffuseMap );
	Crate->Render( PixelLitTexTechnique );

	WorldMatrixVar->SetMatrix( (float*)Ground->GetWorldMatrix() );
    DiffuseMapVar->SetResource( GroundDiffuseMap );
	Ground->Render( PixelLitTexTechnique );

	WorldMatrixVar->SetMatrix( (float*)Stars->GetWorldMatrix() );
    DiffuseMapVar->SetResource( StarsDiffuseMap );
	Stars->Render( PixelLitTexTechnique );

	WorldMatrixVar->SetMatrix( (float*)Light1->GetWorldMatrix() );
    DiffuseMapVar->SetResource( LightDiffuseMap );
	TintColourVar->SetRawValue( Light1Colour, 0, 12 ); // Using special shader that tints the light model to match the light colour
	Light1->Render( AdditiveTexTintTechnique );

	WorldMatrixVar->SetMatrix( (float*)Light2->GetWorldMatrix() );
    DiffuseMapVar->SetResource( LightDiffuseMap );
	TintColourVar->SetRawValue( Light2Colour, 0, 12 );
	Light2->Render( AdditiveTexTintTechnique );
}


// Render everything in the scene
void RenderScene()
{
	//---------------------------
	// Common rendering settings

	// There are some common features all models that we will be rendering, set these once only

	// Pass the camera's matrices to the vertex shader and position to the vertex shader
	ViewMatrixVar->SetMatrix( (float*)&MainCamera->GetViewMatrix() );
	ProjMatrixVar->SetMatrix( (float*)&MainCamera->GetProjectionMatrix() );
	ViewProjMatrixVar->SetMatrix( (float*)&MainCamera->GetViewProjectionMatrix() );
	CameraPosVar->SetRawValue( MainCamera->GetPosition(), 0, 12 );

	// Pass light information to the vertex shader - lights are the same for each model *** and every render target ***
	Light1PosVar->    SetRawValue( Light1->GetPosition(), 0, 12 );  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light1ColourVar-> SetRawValue( Light1Colour, 0, 12 );
	Light2PosVar->    SetRawValue( Light2->GetPosition(), 0, 12 );
	Light2ColourVar-> SetRawValue( Light2Colour, 0, 12 );
	AmbientColourVar->SetRawValue( AmbientColour, 0, 12 );
	SpecularPowerVar->SetFloat( SpecularPower );

	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = g_ViewportWidth;
	vp.Height = g_ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );


	//---------------------------
	// Render scene

	// Select and clear back buffer & depth buffer
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &BackgroundColour[0] );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Render everything
	RenderModels();

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 1, 0 );
}



////////////////////////////////////////////////////////////////////////////////////////
// Window Setup
////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	// Initialise everything in turn
	if( !InitWindow( hInstance, nCmdShow) )
	{
		return 0;
	}
	if( !InitDevice() || !LoadEffectFile() || !InitScene() )
	{
		ReleaseResources();
		return 0;
	}

	// Initialise simple input functions
	InitInput();

	// Initialise a timer class, start it counting now
	CTimer Timer;
	Timer.Start();

	// Main message loop
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		// Check for and deal with window messages (window resizing, minimizing, etc.). When there are none, the window is idle and D3D rendering occurs
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			// Deal with messages
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else // Otherwise render & update
		{
			RenderScene();

			// Get the time passed since the last frame
			float frameTime = Timer.GetLapTime();
			UpdateScene( frameTime );

			if (KeyHit( Key_Escape )) 
			{
				DestroyWindow( HWnd );
			}

		}
	}

	ReleaseResources();

	return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
bool InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
	if( !RegisterClassEx( &wcex ) )	return false;

	// Create window
	HInst = hInstance;
	RECT rc = { 0, 0, 1280, 960 };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	HWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 10: Soft Body Physics", WS_OVERLAPPEDWINDOW,
	                     CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL );
	if( !HWnd )	return false;

	ShowWindow( HWnd, nCmdShow );

	return true;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message )
	{
		case WM_PAINT:
			hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		// These windows messages (WM_KEYXXXX) can be used to get keyboard input to the window
		// This application has added some simple functions (not DirectX) to process these messages (all in Input.cpp/h)
		case WM_KEYDOWN:
			KeyDownEvent( static_cast<EKeyState>(wParam) );
			break;

		case WM_KEYUP:
			KeyUpEvent( static_cast<EKeyState>(wParam) );
			break;
		
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}
