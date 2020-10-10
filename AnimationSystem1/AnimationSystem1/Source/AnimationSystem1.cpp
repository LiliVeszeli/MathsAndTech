/*******************************************
	AnimationSystem1.cpp

	Main window and scene functions
********************************************/

#include <string>
using namespace std;

#include <windows.h>
#include <d3dx9.h>

#include "Defines.h"
#include "BaseMath.h"
#include "CVector3.h"
#include "Mesh.h"
#include "Model.h"
#include "QModel.h"
#include "Animation.h"
#include "Camera.h"
#include "Light.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Array sizes
const int NumMeshes = 4;
const int NumModels = 3;
const int NumQModels = 1;
const int NumRobotAnimations = 2;
const int NumLights = 2;

// Control speed
const float RotSpeed = 0.025f;
const float MoveSpeed = 2.5f;


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Window width and height (client area)
int WindowWidth;
int WindowHeight;
bool movingForward = true;
bool stop = false;

// Core DirectX interface
LPDIRECT3D9             g_pD3D       = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
LPDIRECT3DSURFACE9      g_pd3dRenderSurface = NULL; // Render surface of the main window (back buffer)


// Scene elements
CMesh* Meshes[NumMeshes];
CModel* Models[NumModels];
CQModel* QModels[NumQModels];
CAnimation* RobotAnimations[NumRobotAnimations];
CLight* Lights[NumLights];
CCamera* MainCamera;


//-----------------------------------------------------------------------------
// Scene Constants
//-----------------------------------------------------------------------------

const SColourRGBA AmbientColour( 0.3f, 0.3f, 0.5f, 1.0f );
const SColourRGBA BackgroundColour( 0.5f, 0.9f, 0.9f, 1.0f );

const CVector3 LightCentre = CVector3( 120.0f, 70.0f, 0.0f );
const TFloat32 LightOrbit = 120.0f;
const TFloat32 LightOrbitSpeed = 0.015f;
struct
{
	CVector3    pos;
	SColourRGBA colour;
	TFloat32    bright;
} LightPos[NumLights] = 
{
	{ LightCentre + CVector3(LightOrbit, 0.0f, 0.0f),  SColourRGBA(1.0f, 1.0f, 1.0f),  40.0f },
	{ CVector3(3000.0f, 2000.0f, -5000.0f),            SColourRGBA(1.0f, 0.6f, 0.2f),  5000.0f },
};


//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	/////////////////////////////////
	// Load meshes / create models

	Meshes[0] = new CMesh();
	Meshes[0]->Load( "Stars.x" );

	Meshes[1] = new CMesh();
	Meshes[1]->Load( "Hills.x" );

	Meshes[2] = new CMesh();
	Meshes[2]->Load( "4x4jeep.x" );

	Meshes[3] = new CMesh();
	Meshes[3]->Load( "Robot.x" );

	// Create ordinary matrix-based models - some hills/stars and a car
	Models[0] = new CModel( Meshes[0], CVector3::kOrigin, CVector3(ToRadians(35), -ToRadians(90), 0), CVector3(100, 100, 100) );
	Models[1] = new CModel( Meshes[1], CVector3::kOrigin, CVector3::kZero, CVector3( 4, 2, 4 ) );
	Models[2] = new CModel( Meshes[2], CVector3(80, 0.3f, 0), CVector3(0, ToRadians(-155), 0), CVector3(10, 10, 10) );

	// Create a quaternion-based model - an animatable robot
	QModels[0] = new CQModel( Meshes[3], CVector3(160, 0.3f, 0), CVector3::kZero, CVector3(10, 10, 10) );


	// Load animations
	RobotAnimations[0] = new CAnimation( "RobotWalk", 9, 4 );
	RobotAnimations[1] = new CAnimation( "RobotLook", 5, 2 );

	// Set up initial animations: the parameters are commented by the function code in QModel.cpp
	// There are several optional parameters which allow further animation control
	QModels[0]->PlayAnimation( RobotAnimations[0], 0, true ); // Play the looking animation in slot 0 for the robot


	

	/////////////////////////////
	// Create lights

	for (TUInt32 light = 0; light < NumLights; ++light)
	{
		Lights[light] = new CLight( LightPos[light].pos, LightPos[light].colour, LightPos[light].bright );
		if (!Lights[light])
		{
			return false;
		}
	}
	SetAmbientLight( AmbientColour );
	SetLights( &Lights[0] );


	/////////////////////////////
	// Positioning

	// Set initial camera position and rotations
	MainCamera = new CCamera( CVector3(200, 70, -180), CVector3(ToRadians(15.0f), ToRadians(-25.0f), 0) );
	MainCamera->SetNearFarClip( 1.0f, 5000000.0f ); 

	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release camera
	delete MainCamera;

	// Release lights
	for (TUInt32 light = 0; light < NumLights; ++light)
	{
		delete Lights[light];
	}

	// Release animations
	for (int anim = 0; anim < NumRobotAnimations; ++anim)
	{
		delete RobotAnimations[anim];
	}

	// Release models
	for (TUInt32 model = 0; model < NumQModels; ++model)
	{
		delete QModels[model];
	}
	for (TUInt32 model = 0; model < NumModels; ++model)
	{
		delete Models[model];
	}

	// Release meshes
	for (TUInt32 mesh = 0; mesh < NumMeshes; ++mesh)
	{
		delete Meshes[mesh];
	}

	// Release render methods (shared between meshes)
	ReleaseMethods();
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Render the main models in the scene with given camera and lights
void RenderModels( CCamera* camera )
{
    // Render each main model
	for (TUInt32 model = 0; model < NumModels; ++model)
	{
		// Render model
		Models[model]->Render( camera );
	}
	for (TUInt32 model = 0; model < NumQModels; ++model)
	{
		// Render model
		QModels[model]->Render( camera );
	}
}


// Draw one frame of the scene
void RenderScene()
{
    // Begin the scene
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
		// Clear the back buffer, z buffer & stencil buffer
		g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
							 ToD3DXCOLOR( BackgroundColour ), 1.0f, 0 );

		// Render scene elements
		MainCamera->CalculateMatrices();
		RenderModels( MainCamera );

		// End the scene
        g_pd3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}


// Update the scene between rendering
void UpdateScene()
{
	static bool RotateLight = true;
	static TFloat32 LightBeta = 0.0f;
	static TFloat32 slerp = -1.0f;


	//-----------------------------------------------------
	// Animation Control
	//-----------------------------------------------------

	// Specialised code to blend any second robot animation on an off

	if (KeyHit(Key_Space))
		QModels[0]->PlayAnimation(RobotAnimations[1], 1, false);

	if(KeyHit(Key_R))
	{
		if (movingForward == true)
		{ 
			QModels[0]->PlayAnimation( RobotAnimations[0], 0, true, 1, 1, -1 );
			movingForward = false;
		}
		else
		{
			QModels[0]->PlayAnimation(RobotAnimations[0], 0, true);
			movingForward = true;
		}
	}

	if (KeyHit(Key_Return))
	{
		if (stop == false)
		{
			QModels[0]->PlayAnimation(RobotAnimations[0], 0, true, 1, 1, 0);
			stop = true;
		}
		else
		{
			QModels[0]->PlayAnimation(RobotAnimations[0], 0, true);
			stop = false;
		}
	}
	
	
	
	
	if (QModels[0]->GetAnimation( 1 )) // If second animation is playing...
	{
		// Blend it in and out smoothly (0 weight at start/end, maximum weight in middle)
		TFloat32 normPos = QModels[0]->GetAnimationPosition( 1 ) / 
		                   QModels[0]->GetAnimation( 1 )->GetLength();
		TFloat32 weight = 1.0f - 2.0f * fabsf( normPos - 0.5f);
		QModels[0]->SetAnimationWeight( 1, weight * 3.0f );
	}

	

	// Update animations for each animated model
	for (int model = 0; model < NumQModels; ++model)
	{
		// Pass frame time - no timing in this app so just pass a constant
		QModels[model]->UpdateAnimations( 0.015f );
	}

	//-----------------------------------------------------


	// Move the camera
	MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
	                     MoveSpeed, RotSpeed );

	// Rotate one light
	if (RotateLight)
	{
		Lights[0]->SetPosition( CVector3(LightCentre.x + cos(LightBeta) * LightOrbit, LightCentre.y,
		                                 LightCentre.z + sin(LightBeta) * LightOrbit) );
		LightBeta -= LightOrbitSpeed;
	}
	if (KeyHit( Key_1 ))
	{
		RotateLight = !RotateLight;
	}

	// Move other light on keypresses
	Lights[1]->Control( Key_Numpad8, Key_Numpad2, Key_Numpad4, Key_Numpad6,
	                    Key_Numpad3, Key_Numpad1, MoveSpeed );
}


//-----------------------------------------------------------------------------
// D3D management
//-----------------------------------------------------------------------------

// Initialise Direct3D
bool D3DSetup( HWND hWnd )
{
	// Get client window dimensions
	RECT clientRect;
	GetClientRect( hWnd, &clientRect );
	WindowWidth = clientRect.right;
	WindowHeight = clientRect.bottom;

    // Create the D3D object.
    g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if (!g_pD3D)
	{
        return false;
	}

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;  // Wait for vertical sync
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    // Create the D3DDevice
    if (FAILED(g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                     D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                     &d3dpp, &g_pd3dDevice )))
    {
        return false;
    }
	
	// Turn on tri-linear filtering (for texture, gloss & normal map)
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	return true;
}


// Uninitialise D3D
void D3DShutdown()
{
	// Release D3D interfaces
	if (g_pd3dRenderSurface != NULL)
		g_pd3dRenderSurface->Release();

    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}


} // namespace gen


//-----------------------------------------------------------------------------
// Windows functions - outside of namespace
//-----------------------------------------------------------------------------

// Window message handler
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
		{
            PostQuitMessage( 0 );
            return 0;
		}

		case WM_KEYDOWN:
		{
			gen::EKeyCode eKeyCode = static_cast<gen::EKeyCode>(wParam);
			gen::KeyDownEvent( eKeyCode );
			break;
		}

		case WM_KEYUP:
		{
			gen::EKeyCode eKeyCode = static_cast<gen::EKeyCode>(wParam);
			gen::KeyUpEvent( eKeyCode );
			break;
		}
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

// Windows main function
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), LoadIcon( NULL, IDI_APPLICATION ),
					  LoadCursor( NULL, IDC_ARROW ), NULL, NULL,
                      "AnimationSystem1", NULL };
    RegisterClassEx( &wc );

    // Create the application's window
    HWND hWnd = CreateWindow( "AnimationSystem1", "CO3303 - Simple Animation System",
                              WS_OVERLAPPEDWINDOW, 100, 20, 1280, 960,
                              NULL, NULL, wc.hInstance, NULL );

    // Initialize Direct3D
	if (gen::D3DSetup( hWnd ))
    {
        // Prepare the scene
        if (gen::SceneSetup())
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof(msg) );
            while( msg.message != WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
				{
					// Render and update the scene
                    gen::RenderScene();
					gen::UpdateScene();
					if (gen::KeyHeld( gen::Key_Escape ))
					{
						DestroyWindow( hWnd );
					}
				}
            }
        }
	    gen::SceneShutdown();
    }
	gen::D3DShutdown();

	UnregisterClass( "AnimationSystem1", wc.hInstance );
    return 0;
}
