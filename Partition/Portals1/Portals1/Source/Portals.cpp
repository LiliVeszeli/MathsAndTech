/*******************************************
	Portals.cpp

	Shell scene and game functions
********************************************/

#include <vector>
#include <sstream>
#include <string>
using namespace std;

#include <d3dx9.h>

#include "Defines.h"
#include "CVector3.h"
#include "CVector4.h"
#include "CMatrix4x4.h"
#include "RenderMethod.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Portals.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Game / scene constants
//-----------------------------------------------------------------------------

// Fixed array sizes
const int NumPartitions = 7;
const int MaxPortals = 5;
const int NumCars = 10;
const int NumLights = 1;

// Control speed
const float CameraRotSpeed = 2.0f;
float CameraMoveSpeed = 5.0f;


//-----------------------------------------------------------------------------
// Global system variables
//-----------------------------------------------------------------------------

// Get reference to global DirectX variables from another source file
extern LPDIRECT3DDEVICE9 g_pd3dDevice;
extern LPD3DXFONT        g_pFont;

// Actual viewport dimensions (fullscreen or windowed)
extern TUInt32 ViewportWidth;
extern TUInt32 ViewportHeight;

// Current mouse position
extern TInt32 MouseX;
extern TInt32 MouseY;


//-----------------------------------------------------------------------------
// Global game/scene variables
//-----------------------------------------------------------------------------

// Entity manager
CEntityManager EntityManager;

// List of car UIDs
TEntityUID Cars[NumCars];

// Other scene elements
SColourRGBA AmbientLight;
CLight* Lights[NumLights];
CCamera* MainCamera;


/********************************
	Partition data
********************************/

// Portal structure
struct SPortal
{
	int  PortalPoly;     // Index of the portal polygon (it's points) in array below
	bool FrontVisible;   // Need to see front side of portal for it to be visible (otherwise back)
	int  OtherPartition; // The partition linked to by this portal
};


// Space partition structure
struct SPartition
{
	// Partition bounds - all partitions are cuboids in this simple example
	float MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	// The potentially visible set of partitions
	int PVSSize; // Number of entries in array
	int PVS[NumPartitions];

	// The portals within this partition, using SPortal structure above
	int NumPortals; // Number of portals in array
	SPortal Portals[MaxPortals];

	// Whether this portal has already been rendered this frame - needed for portal algorithm
	bool Rendered;

	// A vector (dynamic array) of the entities contained in this partition
	vector<TEntityUID> Entities;
};


// Array of partitions - partitions A-G numbered 0-6 here in the code
// Initialise all data except the entity list
SPartition Partitions[NumPartitions] = 
{
	{ // Partition A (0)
		-500.0f, 500.0f,  0.0f, 1000.0f,  -500.0f, 500.0f, // Partition bounds
		// PVS list - currently empty. Entries should be in the form 2, {1, 5} to indicate
		// 2 potentially visible partitions 1 & 5
		0, {}, 
		// 5 portals - first portal uses poly "0" from array below, the *back* of the polygon
		// is the portal's visible side ("false") and it leads to partition "1". Etc.
		5, { {0, false, 1}, {1, false, 6}, {6, false, 2}, {7, true, 3}, {8, true, 3} }, 
	},

	{ // Partition B (1)
		-2.50f, -0.50f,  -20.0f, 2.55f,  -4.70f, -3.15f,
		0, {}, 
		2, { {0, true, 0}, {2, false, 2} },
	},

	{ // Partition C (2)
		-3.60f, 0.45f,  -20.0f, 3.05f,  -3.15f, -0.05f,
		0, {}, 
		3, { {2, true, 1}, {3, false, 3}, {6, true, 0} },
	},

	{ // Partition D (3)
		-3.60f, 3.55f,  -20.0f, 3.05f,  -0.05f, 4.00f,
		0, {}, 
		4, { {3, true, 2}, {4, false, 4}, {7, false, 0}, {8, false, 0} },
	},

	{ // Partition E (4)
		3.55f, 5.60f,  -20.0f, 3.05f,  -3.10f, 4.00f,
		0, {}, 
		2, { {4, true, 3}, {5, false, 5} },
	},

	{ // Partition F (5)
		0.45f, 3.55f,  -20.0f, 3.05f,  -3.10f, -0.05f,
		0, {}, 
		1, {5, true, 4},
	},

	{ // Partition G (6)
		-9.50f, -8.00f,  -20.0f, 2.55f,  -4.00f, -1.00f,
		0, {}, 
		1, {1, true, 0},
	},
};


// Define a portal polygon - assuming all portals are quads, so always 4 points
typedef CVector3 TPortalPoly[4];

// Array of portal polygons in the scene
TPortalPoly PortalPolys[] = 
{
	{ CVector3(-2.05f,  0.0f, -4.75f), CVector3(-0.95f,  0.0f, -4.75f), // Door A-B
	  CVector3(-0.95f,  2.1f, -4.75f), CVector3(-2.05f,  2.1f, -4.75f) },
	{ CVector3(-7.975f, 0.0f, -3.8f),  CVector3(-7.975f, 0.0f, -2.7f), // Door A-G
	  CVector3(-7.975f, 2.1f, -2.7f),  CVector3(-7.975f, 2.1f, -3.8f) },
	{ CVector3(-2.05f,  0.0f, -3.15f), CVector3(-0.95f,  0.0f, -3.15f), // Door B-C
	  CVector3(-0.95f,  2.1f, -3.15f), CVector3(-2.05f,  2.1f, -3.15f) },
	{ CVector3(-1.05f,  0.0f, -0.05f), CVector3(-0.05f,  0.0f, -0.05f), // Door C-D
	  CVector3( 0.05f,  2.1f, -0.05f), CVector3(-1.05f,  2.1f, -0.05f) },
	{ CVector3( 3.55f,  0.0f,  2.6f),  CVector3( 3.55f,  0.0f,  1.5f),  // Door D-E
	  CVector3( 3.55f,  2.1f,  1.5f),  CVector3( 3.55f,  2.1f,  2.6f) },
	{ CVector3( 3.55f,  0.0f, -2.0f),  CVector3( 3.55f,  0.0f, -0.9f),  // Door E-F
	  CVector3( 3.55f,  2.1f, -0.9f),  CVector3( 3.55f,  2.1f, -2.0f) },
		
	{ CVector3(-3.65f, 0.75f, -0.7f),  CVector3(-3.65f, 0.75f, -2.3f),  // Window A-C
	  CVector3(-3.65f, 2.35f, -2.3f),  CVector3(-3.65f, 2.35f, -0.7f) },
	{ CVector3( 0.95f, 0.75f,  4.05f), CVector3( 2.55f, 0.75f,  4.05f), // Window A-D 1
	  CVector3( 2.55f, 2.35f,  4.05f), CVector3( 0.95f, 2.35f,  4.05f) },
	{ CVector3(-2.55f, 0.75f,  4.05f), CVector3(-0.95f, 0.75f,  4.05f), // Window A-D 2
	  CVector3(-0.95f, 2.35f,  4.05f), CVector3(-2.55f, 2.35f,  4.05f) }
};


//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////
	// Create scenery templates and entities

	// Create scenery templates - loads the meshes
	// Template type, template name, mesh name
	EntityManager.CreateTemplate( "Scenery", "Skybox", "Skybox.x" );
	EntityManager.CreateTemplate( "Scenery", "Floor", "Floor.x" );
	EntityManager.CreateTemplate( "Scenery", "House", "House.x" );
	EntityManager.CreateTemplate( "Scenery", "Shed", "Shed.x" );
	EntityManager.CreateTemplate( "Scenery", "Room B", "RoomB.x" );
	EntityManager.CreateTemplate( "Scenery", "Room C", "RoomC.x" );
	EntityManager.CreateTemplate( "Scenery", "Room D", "RoomD.x" );
	EntityManager.CreateTemplate( "Scenery", "Room E", "RoomE.x" );
	EntityManager.CreateTemplate( "Scenery", "Room F", "RoomF.x" );
	EntityManager.CreateTemplate( "Scenery", "Room G", "RoomG.x" );
	EntityManager.CreateTemplate( "Scenery", "Door A-B", "DoorA-B.x" );
	EntityManager.CreateTemplate( "Scenery", "Door A-G", "DoorA-G.x" );
	EntityManager.CreateTemplate( "Scenery", "Door B-C", "DoorB-C.x" );
	EntityManager.CreateTemplate( "Scenery", "Door C-D", "DoorC-D.x" );
	EntityManager.CreateTemplate( "Scenery", "Door D-E", "DoorD-E.x" );
	EntityManager.CreateTemplate( "Scenery", "Door E-F", "DoorE-F.x" );
	EntityManager.CreateTemplate( "Scenery", "Window A-C", "WindowA-C.x" );
	EntityManager.CreateTemplate( "Scenery", "Window A-D 1", "WindowA-D1.x" );
	EntityManager.CreateTemplate( "Scenery", "Window A-D 2", "WindowA-D2.x" );

	// Create scenery entities, add each to the appropriate partition
	// All added to partition A at first - you must change to the correct partitions
	// Note that template name = entity name for many entities here since each template has only
	// one entity instance
	TEntityUID id;
	id = EntityManager.CreateEntity( "Skybox", "Skybox", CVector3(0.0f, -1000.0f, 0.0f) );
	Partitions[0].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Floor", "Floor" );
	Partitions[0].Entities.push_back( id );
	id = EntityManager.CreateEntity( "House", "House" );
	Partitions[0].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Shed", "Shed" );
	Partitions[0].Entities.push_back( id );

	id = EntityManager.CreateEntity( "Room B", "Room B" );
	Partitions[1].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Room C", "Room C" );
	Partitions[2].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Room D", "Room D" );
	Partitions[3].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Room E", "Room E" );
	Partitions[4].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Room F", "Room F" );
	Partitions[5].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Room G", "Room G" );
	Partitions[6].Entities.push_back( id );

	id = EntityManager.CreateEntity( "Door A-B", "Door A-B" );
	Partitions[0].Entities.push_back( id );
	Partitions[1].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Door A-G", "Door A-G" );
	Partitions[0].Entities.push_back( id );
	Partitions[6].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Door B-C", "Door B-C" );
	Partitions[1].Entities.push_back( id );
	Partitions[2].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Door C-D", "Door C-D" );
	Partitions[2].Entities.push_back( id );
	Partitions[3].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Door D-E", "Door D-E" );
	Partitions[3].Entities.push_back( id );
	Partitions[4].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Door E-F", "Door E-F" );
	Partitions[4].Entities.push_back( id );
	Partitions[5].Entities.push_back(id);

	id = EntityManager.CreateEntity( "Window A-C", "Window A-C" );
	Partitions[0].Entities.push_back( id );
	Partitions[2].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Window A-D 1", "Window A-D 1" );
	Partitions[0].Entities.push_back( id );
	Partitions[3].Entities.push_back(id);
	id = EntityManager.CreateEntity( "Window A-D 2", "Window A-D 2" );
	Partitions[0].Entities.push_back( id );
	Partitions[3].Entities.push_back(id);

	
	/////////////////////////////////
	// Create car templates

	// Template type, template name, mesh name, top speed, acceleration, turn speed
	EntityManager.CreateCarTemplate( "Car", "Freelander", "4x4jeep.x", 48.0f, 2.2f, 2.0f );
	EntityManager.CreateCarTemplate( "Car", "Aston Martin", "amartin.x", 61.0f, 2.8f, 1.4f );
	EntityManager.CreateCarTemplate( "Car", "Fiat Panda", "FiatPanda.x", 42.0f, 2.0f, 3.2f );
	EntityManager.CreateCarTemplate( "Car", "Intrepid", "Intrepid.x", 55.0f, 2.6f, 1.7f );
	EntityManager.CreateCarTemplate( "Car", "Transit Van", "TransitVan.x", 48.0f, 2.1f, 2.2f );


	////////////////////////////////
	// Create car entities

	// Type (template name), entity name, position, rotation
	Cars[0] = EntityManager.CreateCar( "Freelander", "A", CVector3(0.0f, 0.0f, 20.0f),
	                                   CVector3(0.0f, ToRadians(0.0f), 0.0f) );
	Cars[1] = EntityManager.CreateCar( "Freelander", "B", CVector3(0.0f, 0.0f, -20.0f),
	                                   CVector3(0.0f, ToRadians(180.0f), 0.0f) );
	Cars[2] = EntityManager.CreateCar( "Aston Martin", "C", CVector3(11.76f, 0.0f, 16.18f),
	                                   CVector3(0.0f, ToRadians(36.0f), 0.0f) );
	Cars[3] = EntityManager.CreateCar( "Aston Martin", "D", CVector3(-11.76f, 0.0f, -16.18f),
	                                   CVector3(0.0f, ToRadians(216.0f), 0.0f) );
	Cars[4] = EntityManager.CreateCar( "Fiat Panda", "E", CVector3(19.02f, 0.0f, 6.18f),
	                                   CVector3(0.0f, ToRadians(72.0f), 0.0f) );
	Cars[5] = EntityManager.CreateCar( "Fiat Panda", "F", CVector3(-19.02f, 0.0f, -6.18f),
	                                   CVector3(0.0f, ToRadians(252.0f), 0.0f) );
	Cars[6] = EntityManager.CreateCar( "Intrepid", "G", CVector3(19.02f, 0.0f, -6.18f),
	                                   CVector3(0.0f, ToRadians(108.0f), 0.0f) );
	Cars[7] = EntityManager.CreateCar( "Intrepid", "H", CVector3(-19.02f, 0.0f, 6.18f),
	                                   CVector3(0.0f, ToRadians(288.0f), 0.0f) );
	Cars[8] = EntityManager.CreateCar( "Transit Van", "I", CVector3(11.76f, 0.0f, -16.18f),
	                                   CVector3(0.0f, ToRadians(144.0f), 0.0f) );
	Cars[9] = EntityManager.CreateCar( "Transit Van", "J", CVector3(-11.76f, 0.0f, 16.18f),
	                                   CVector3(0.0f, ToRadians(324.0f), 0.0f) );

	// Add all car entities to partition A
	for (int car = 0; car < NumCars; ++car)
	{
		Partitions[0].Entities.push_back( Cars[car] );
	}


	/////////////////////////////
	// Camera / light setup

	// Set camera position and clip planes
	MainCamera = new CCamera( CVector3( 0.0f, 8.0f, -20.0f ),
	                          CVector3(ToRadians(25.0f), 0, 0) );
	MainCamera->SetNearFarClip( 0.1f, 10000.0f ); 


	// Ambient light level
	AmbientLight = SColourRGBA( 0.6f, 0.6f, 0.6f, 1.0f );

	// Sunlight
	Lights[0] = new CLight( CVector3( -1000.0f, 800.0f, -2000.0f),
	                        SColourRGBA(1.0f, 0.9f, 0.2f), 4000.0f );

	// Set light information for render methods
	SetAmbientLight( AmbientLight );
	SetLights( &Lights[0] );

	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release render methods
	ReleaseMethods();

	// Release lights
	for (int light = NumLights - 1; light >= 0; --light)
	{
		delete Lights[light];
	}

	// Release camera
	delete MainCamera;

	// Destroy all entities / templates
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Partition support functions
//-----------------------------------------------------------------------------

// Return the partition number that the given point is in. This is a typical
// requirement for portal-based systems. In this simple example, the partitions
// are all cuboids (except the first one), so we can just test the point against
// the bounds of each partitions. In a more complex system, a different, more
// efficient system is needed - e.g. using an oct-tree to look up the partition
int GetPartitionFromPt( CVector3 pt )
{
	// Go through each partition except the first...
	for (int part = 1; part < NumPartitions; ++part)
	{
		// Test if point is within bounds of the partition
		if (pt.x > Partitions[part].MinX && pt.x < Partitions[part].MaxX && 
			pt.y > Partitions[part].MinY && pt.y < Partitions[part].MaxY && 
			pt.z > Partitions[part].MinZ && pt.z < Partitions[part].MaxZ)
		{
			return part;
		}
	}

	// Not in any other partition, must be in partition 0 (partition A - the world)
	return 0;
}


// Render all the instances in the given partition number with the given camera
void RenderPartition( int part, CCamera* camera )
{
	// Step through entity vector, rendering each one
	vector<TEntityUID>::iterator itEntity;
	itEntity = Partitions[part].Entities.begin();
	while (itEntity != Partitions[part].Entities.end())
	{
		EntityManager.GetEntity( *itEntity )->Render( camera );
		++itEntity;
	}

	// Mark partition as rendered
	Partitions[part].Rendered = true;
}


// Check visiblity of portals in a partition - if any are visible in the given screen area
// and camera, then render them, and then recursively render their visible portals
// Doesn't deal with portals clipped at the near plane => errors when very close to portals
void RenderPortals( int part, CCamera* camera, int minX, int minY, int maxX, int maxY )
{
	// For each portal of given partition
	for (int portal = 0; portal < Partitions[part].NumPortals; ++portal)
	{
		// Get polygon for this portal
		int poly = Partitions[part].Portals[portal].PortalPoly;

		// Get vector from portal to camera
		CVector3 portalCamera;
		//++++ MISSING - initialise variable 'portalCamera'

		// Get facing vector of portal
		CVector3 portalVec0 = PortalPolys[poly][1] - PortalPolys[poly][0];
		CVector3 portalVec1 = PortalPolys[poly][2] - PortalPolys[poly][1];
		CVector3 portalFacing = Cross( portalVec0, portalVec1 );
		
		// Check if portal is facing correct way to be visible before continuing
		float facing = Dot( portalCamera, portalFacing );
		if (Partitions[part].Portals[portal].FrontVisible && facing > 0.0f ||
		    !Partitions[part].Portals[portal].FrontVisible && facing < 0.0f)
		{
			// Convert each portal point to screen coords, keep track of min and max coords
			int portalMinX = ViewportWidth, portalMaxX = -1; // Set initial off-screen values so
			int portalMinY = ViewportHeight, portalMaxY = -1;// portal fails if no pts on screen
			for (int pt = 0; pt < 4; ++pt)
			{
				// Convert portal coordinate to screen pt - if possible
				int x, y;
				if (camera->PixelFromWorldPt( PortalPolys[poly][pt], ViewportWidth,
											  ViewportHeight, &x, &y ))
				{
					// Test portal screen point against current min/max
					if (x < portalMinX)
					{
						portalMinX = x;
					}
					if (x > portalMaxX)
					{
						//++++ MISSING
					}
					if (y < portalMinY)
					{
						//++++ MISSING
					}
					if (y > portalMaxY)
					{
						//++++ MISSING
					}
				}
			}

			// Min and max coordinates form a screen bounding rectangle for the current
			// portal - see if it intersects with the screen area passed to the function
			if (portalMaxX > minX && portalMinX < maxX && portalMaxY > minY && portalMinY < maxY)
			{
				// The portal can be seen. Calculate the screen area where it is visible - the
				// intersection of the portal rectangle and the screen area being tested
				int newMinX = Max( minX, portalMinX );
				int newMaxX = Min( maxX, portalMaxX );
				int newMinY = Max( minY, portalMinY );
				int newMaxY = Min( maxY, portalMaxY );

				// Get target partition of portal
				int newPart = Partitions[part].Portals[portal].OtherPartition;
				if (!Partitions[newPart].Rendered)
				{
					// Render the new partition
					//++++ MISSING

					// Render portals of new partition if visible in the new screen area
					//++++ MISSING
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
    // Begin the scene
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
		// Clear the back buffer, z buffer & stencil buffer
		g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
		                     0, 1.0f, 0 );

		// Prepare camera
		MainCamera->CalculateMatrices();


		/********************************
			Partition rendering
		********************************/

		// Mark all partitions as not rendered
		for (int part = 0; part < NumPartitions; ++part)
		{
			Partitions[part].Rendered = false;
		}

		// Find starting partition - always choosing 0 here - add correct code for exercise
		int start = 0; 

		// Render all entities in this partition
		RenderPartition( GetPartitionFromPt(MainCamera->Position()), MainCamera );


		//****** Render PVS here ********
		// Go through this partition's PVS and render each partition in it


		//****** Render Portals here ********
		// Render those partitions visible (in the viewport) through portals in
		// the starting partition


		// Draw on-screen text
		RenderSceneText( updateTime );

		// End the scene
        g_pd3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}


// Render on-screen text each frame
void RenderSceneText( float updateTime )
{
	// Write FPS text string
	stringstream outText;
	outText << "Frame Time: " << updateTime * 1000.0f << "ms" << endl 
	        << "FPS:" << 1.0f / updateTime;
	RECT rect;
	SetRect( &rect, 0, 0, 0, 0 );  // Top/left of text at (0,0), don't need bottom/right (DT_NOCLIP)
	g_pFont->DrawText( NULL, outText.str().c_str(), -1, &rect, DT_NOCLIP,
	                   D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
	outText.str("");


	// Display rendered partitions
	outText << "Partitions Rendered: ";
	for (int part = 0; part < NumPartitions; ++part)
	{
		if (Partitions[part].Rendered)
		{
			outText << part << " ";
		}
	}
	SetRect( &rect, 0, 40, 0, 0 );  // Top/left of text at (0,0), don't need bottom/right (DT_NOCLIP)
	g_pFont->DrawText( NULL, outText.str().c_str(), -1, &rect, DT_NOCLIP,
	                   D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
}


// Update the scene between rendering
void UpdateScene( float updateTime )
{
	// Call all entity update functions
	EntityManager.UpdateAllEntities( updateTime );

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit( Key_F2 )) CameraMoveSpeed = 5.0f;
	if (KeyHit( Key_F3 )) CameraMoveSpeed = 40.0f;

	// Move the camera
	MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, 
	                     CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime );
}


//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Select a random car and return its UID, needed for the car behaviour
TEntityUID RandomCar()
{
	return Cars[Random(0, NumCars-1)];
}


} // namespace gen
