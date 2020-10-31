/*******************************************
	MainApp.cpp

	Windows functions and DirectX setup
********************************************/

#include <windows.h>
#include <windowsx.h>
#include <d3dx9.h>

#include "Defines.h"
#include "Input.h"
#include "CTimer.h"
#include "Portals.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Core DirectX interface
LPDIRECT3D9       g_pD3D       = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL; // Our rendering device
LPD3DXFONT        g_pFont      = NULL;  // D3DX font

// Window rectangle (dimensions) & client window rectangle - used for toggling fullscreen
RECT ClientRect;
RECT WindowRect;
bool Fullscreen;

// Actual viewport dimensions (fullscreen or windowed)
TUInt32 ViewportWidth;
TUInt32 ViewportHeight;

// Current mouse position
TInt32 MouseX;
TInt32 MouseY;

// Game timer
CTimer Timer;


//-----------------------------------------------------------------------------
// D3D management
//-----------------------------------------------------------------------------

// Initialise Direct3D
bool D3DSetup( HWND hWnd )
{
	// Get initial window and client window dimensions
	GetWindowRect( hWnd, &WindowRect );
	GetClientRect( hWnd, &ClientRect );

    // Create the D3D object.
    g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if (!g_pD3D)
	{
        return false;
	}

    // Set up the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;  // Don't wait for vertical sync
	d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.Windowed = TRUE;
	d3dpp.BackBufferWidth = ClientRect.right;
	d3dpp.BackBufferHeight = ClientRect.bottom;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	Fullscreen = false;
	ViewportWidth = d3dpp.BackBufferWidth;
	ViewportHeight = d3dpp.BackBufferHeight;

    // Create the D3DDevice
    if (FAILED(g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                     D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                     &d3dpp, &g_pd3dDevice )))
    {
        return false;
    }
	
	// Turn on tri-linear filtering (for up to three simultaneous textures)
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	// Create a font using D3DX helper functions
    if (FAILED(D3DXCreateFont( g_pd3dDevice, 12, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &g_pFont )))
    {
        return false;
    }

	return true;
}


// Reset the Direct3D device to resize window or toggle fullscreen/windowed
bool ResetDevice( HWND hWnd, bool ToggleFullscreen = false )
{
	// If currently windowed...
	if (!Fullscreen)
	{
		// Get current window and client window dimensions
		RECT NewClientRect;
		GetWindowRect( hWnd, &WindowRect );
		GetClientRect( hWnd, &NewClientRect );

		// If not switching to fullscreen, then we must ensure the window is changing size, if
		// it isn't then return without doing anything
		if (!ToggleFullscreen && NewClientRect.right == ClientRect.right && 
			NewClientRect.bottom == ClientRect.bottom)
		{
			return true;
		}
		ClientRect = NewClientRect;
	}

	// Toggle fullscreen if requested
	if (ToggleFullscreen)
	{
		Fullscreen = !Fullscreen;
	}

	// Reset the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;  // Don't wait for vertical sync
	d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	if (!Fullscreen)
	{
		// Set windowed parameters - need to set the back buffer dimensions when reseting,
		// match them to the window client area
	    d3dpp.Windowed = TRUE;
		d3dpp.BackBufferWidth = ClientRect.right;
		d3dpp.BackBufferHeight = ClientRect.bottom;
	    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	}
	else
	{
		// Get current dimensions of primary monitor
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo( g_pD3D->GetAdapterMonitor( D3DADAPTER_DEFAULT ), &monitorInfo ))
		{
			d3dpp.BackBufferWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			d3dpp.BackBufferHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
		}
		else
		{
			d3dpp.BackBufferWidth = 1280;
			d3dpp.BackBufferHeight = 1024;
		}

		// Set fullscreen parameters
		d3dpp.Windowed = FALSE;
	    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	}
	ViewportWidth = d3dpp.BackBufferWidth;
	ViewportHeight = d3dpp.BackBufferHeight;

	// Need to recreate resources when reseting. Any resources (vertex & index buffers, textures) created
	// using D3DPOOL_MANAGED rather than D3DPOOL_DEFAULT will be recreated automatically. Dynamic resources
	// must be in D3DPOOL_DEFAULT, so they must be recreated manually. D3DX fonts are such an example.
	// Other dynamic resources are those that are updated during the game loop, e.g. procedural textures,
	// or dynamic terrain
	if (g_pFont != NULL)
		g_pFont->Release();

	// Reset the Direct3D device with the new settings
    if (FAILED(g_pd3dDevice->Reset( &d3dpp )))
    {
        return false;
    }

	// If reseting to windowed mode, we need to reset the window size
	if (!Fullscreen)
	{
		SetWindowPos( hWnd, HWND_NOTOPMOST, WindowRect.left, WindowRect.top,
			          WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, 0 );
	}

	// Need to set up states again after reset
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	// Recreate the font
    if (FAILED(D3DXCreateFont( g_pd3dDevice, 12, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &g_pFont )))
    {
        return false;
    }

	return true;
}


// Uninitialise D3D
void D3DShutdown()
{
	// Release D3D interfaces
	if (g_pFont != NULL)
		g_pFont->Release();

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

        case WM_SIZE:
		{
			// Resized window - reset device to match back buffer to new window size
			if (gen::g_pd3dDevice && !gen::ResetDevice( hWnd ))
			{
				DestroyWindow( hWnd );
			}
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
		case WM_MOUSEMOVE:
		{
			gen::MouseX = GET_X_LPARAM(lParam); 
			gen::MouseY = GET_Y_LPARAM(lParam);
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
                      "Portals", NULL };
    RegisterClassEx( &wc );

    // Create the application's window
	HWND hWnd = CreateWindow( "Portals", "Partitions, PVS & Portals",
                              WS_OVERLAPPEDWINDOW, 100, 100, 1280, 960,
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

			// Reset the timer for a timed game loop
			gen::Timer.Reset();

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
					// Render and update the scene - using variable timing
					float updateTime = gen::Timer.GetLapTime();
                    gen::RenderScene( updateTime );
					gen::UpdateScene( updateTime );

					// Toggle fullscreen / windowed
					if (gen::KeyHit( gen::Key_F1 ))
					{
						if (!gen::ResetDevice( hWnd, true ))
						{
							DestroyWindow( hWnd );
						}
					}

					// Quit on escape
					if (gen::KeyHit( gen::Key_Escape ))
					{
						DestroyWindow( hWnd );
					}
				}
            }
        }
	    gen::SceneShutdown();
    }
	gen::D3DShutdown();

	UnregisterClass( "Portals", wc.hInstance );
    return 0;
}
