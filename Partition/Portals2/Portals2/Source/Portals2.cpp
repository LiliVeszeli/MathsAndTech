/*******************************************
	Portals.cpp

	Shell scene and game functions
********************************************/

#include <vector>
#include <list>
#include <sstream>
#include <string>
using namespace std;

#include <d3dx9.h>

#include "Defines.h"
#include "CVector3.h"
#include "CVector4.h"
#include "CMatrix4x4.h"
#include "MathDX.h"
#include "RenderMethod.h"
#include "Mesh.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Portals2.h"

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

// Depth of portal recursion, used for stencil buffer work
int PortalDepth;


/********************************
	Portal Shape Types & Data
********************************/

// Define an enumeration to refer to the shapes below in a more readable way
enum EPortalShape
{
	Door = 0,
	Window,
	Taper,
	Diamond,
	NumPortalShapes // Leave this entry at the end to keep its value correct
};


// Define the type TPortalShape to represent a quad of points - a portal shape
typedef CVector3 TPortalShape[4];

// In this exercise portal polygons are chosen from a selection of shapes, defined here.
// These portal shapes are defined in *model* space (and are actually turned into meshes,
// which are used to clear the depth buffer when rendering a portal). Each portal selects
// a shape and positions it in the world with a world matrix. The vertices of each portal
// shape must be ordered clockwise when viewed from the entrance partition.
// Portal shapes here are limited to quads, four different shapes are defined here.
TPortalShape PortalShapes[] =
{
	{ CVector3(-0.55f, 0.0f, 0.0f), CVector3(-0.55f, 2.1f, 0.0f),   // Door shape
	  CVector3( 0.55f, 2.1f, 0.0f), CVector3( 0.55f, 0.0f, 0.0f) },
	{ CVector3(-0.8f, 0.0f, 0.0f), CVector3(-0.8f, 1.6f, 0.0f),     // Window shape
	  CVector3( 0.8f, 1.6f, 0.0f), CVector3( 0.8f, 0.0f, 0.0f) },
	{ CVector3(-0.7f, 0.0f, 0.0f), CVector3(-0.3f, 2.1f, 0.0f),     // Tapered portal shape
	  CVector3( 0.3f, 2.1f, 0.0f), CVector3( 0.7f, 0.0f, 0.0f) },
	{ CVector3( 0.0f, 0.0f, 0.0f), CVector3(-0.6f, 1.05f, 0.0f),    // Diamond portal shape
	  CVector3( 0.0f, 2.1f, 0.0f), CVector3( 0.6f, 1.05f, 0.0f) },
};


// Portals are rendered in two passes - using a combination of render methods that clear the depth
// buffer in the portal area. This allows portals to be placed anywhere in the scene - they
// will "cut holes" in existing geometry
CMesh* PortalMeshes[NumPortalShapes];

// Indices for portals when used as meshes (basic triangle list for a quad)
TUInt16 PortalIndices[6] = { 0, 1, 2,  0, 2, 3 };


/********************************
	Portal Types & Data
********************************/

// Portal structure - more advanced than the previous exercise
// Contains two portal polygons, one for each side of the portal - this allows the portal
// entrance and exit to be in different places in the scene. The portal shapes are defined
// from the list above, and positioned with (world) matrices. Can also specify if the portal
// "cuts out" holes in the geometry (rather than representing an existing hole)
struct SPortal
{
	int        Shape;        // Index of the portal shape in the array above
	CMatrix4x4 InMatrix;     // World matrix to position the portal "entrance"
	CMatrix4x4 OutMatrix;    // World matrix to position the portal "exit"
	int        InPartition;  // The partition containing the "entrance"
	int        OutPartition; // The partition containing the "exit"
};

// A global list of all the portals in the scene. Define a couple of types to improve readability
typedef list<SPortal*>        TPortalList;
typedef TPortalList::iterator TPortalIter;
TPortalList Portals;


/********************************
	Partition Types & Data
********************************/

// Space partition structure
struct SPartition
{
	// Partition bounds - all partitions are cuboids in this simple example
	float MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	// Whether this portal has already been rendered this frame
	bool Rendered;

	// A vector (dynamic array) of the entities contained in this partition
	vector<TEntityUID> Entities;

	// A list of the portals contained in this partition. Poiinters to portals held in the
	// main portal list above - each portal is shared between two partitions
	TPortalList Portals;
};


// Array of partitions - partitions A-G numbered 0-6 here in the code
// Initialise partition bounds only
SPartition Partitions[NumPartitions] = 
{
	{ -500.0f, 500.0f,   0.0f,  1000.0f,  -500.0f, 500.0f, }, // Partition A (0)
	{ -2.50f, -0.50f,   -20.0f, 2.55f,    -4.70f, -3.15f, },  // Partition B (1)
	{ -3.60f,  0.45f,   -20.0f, 3.05f,    -3.15f, -0.05f, },  // Partition C (2)
	{ -3.60f,  3.55f,   -20.0f, 3.05f,    -0.05f,  4.00f, },  // Partition D (3)
	{  3.55f,  5.60f,   -20.0f, 3.05f,    -3.10f,  4.00f, },  // Partition E (4)
	{  0.45f,  3.55f,   -20.0f, 3.05f,    -3.10f, -0.05f, },  // Partition F (5)
	{ -9.50f, -8.00f,   -20.0f, 2.55f,    -4.00f, -1.00f, },  // Partition G (6)
};


//-----------------------------------------------------------------------------
// Partition Functions
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


//-----------------------------------------------------------------------------
// Portal Shape Functions
//-----------------------------------------------------------------------------

// Transform a given portal shape (an index into the "PortalShapes" array above) by a given
// world matrix, returning the transformed shape in the parameter "transformedShape"
void TransformPortalShape( int shapeIndex, CMatrix4x4 worldMatrix, TPortalShape transformedShape )
{
	transformedShape[0] = worldMatrix.TransformPoint( PortalShapes[shapeIndex][0] );
	transformedShape[1] = worldMatrix.TransformPoint( PortalShapes[shapeIndex][1] );
	transformedShape[2] = worldMatrix.TransformPoint( PortalShapes[shapeIndex][2] );
	transformedShape[3] = worldMatrix.TransformPoint( PortalShapes[shapeIndex][3] );
}

// Get the facing vector (normal) to a given portal polygon (shape)
CVector3 GetPortalPolyNormal( TPortalShape portalPoly )
{
	// Cross product of two edges in a polygon gives a vector at right angles to that polygon
	CVector3 portalVec0 = portalPoly[1] - portalPoly[0];
	CVector3 portalVec1 = portalPoly[2] - portalPoly[1];
	return Normalise( Cross( portalVec0, portalVec1 ) );
}


//-----------------------------------------------------------------------------
// Portal Rendering
//-----------------------------------------------------------------------------

// Render the portal shape before rendering the target partition. This stage clears any existing
// pixels and limits subsequent rendering to this area using stencil buffer and z-buffer techniques
// Similar to (recursive) stencil mirror code
void PreRenderPortalShape( int shape, CMatrix4x4 matrix, CCamera* camera )
{
	// Render the portal shape, increasing values in the stencil buffer where it is visible.
	// Also clears the colour of the portal area. No culling as cw/ccw order of portal vertices
	// is different for entrance and exit
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	g_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR );
	PortalMeshes[shape]->Render( &matrix, camera, PlainColour );

	// Increase the portal depth and stencil match value - from here on will only render in the
	// pixels set above - limits further partition rendering to within the portal shape only
	++PortalDepth;
	g_pd3dDevice->SetRenderState( D3DRS_STENCILREF, PortalDepth );

	// Set the z-buffer values to 1.0f within the portal shape, 1.0f is the most distant z value.
	// Have now fully cleared the area of the portal shape as well as setting unique stencil values
	g_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
	g_pd3dDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
	PortalMeshes[shape]->Render( &matrix, camera, ClearDepth );

	// Restore normal states
	g_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}

// Render the portal shape after rendering of the target partition. This stage resets the stencil
// buffer values in the portal area, undoing above function
void PostRenderPortalShape( int shape, CMatrix4x4 matrix, CCamera* camera )
{
	// Decrease the portal depth and stencil match value, reseting step from above function
	--PortalDepth;
	g_pd3dDevice->SetRenderState( D3DRS_STENCILREF, PortalDepth );

	// Switch off culling as in above function, prepare to write all stencil values
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	g_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );

	// Use alpha blending to draw nothing (invisible surface), but the stencil will be written to
	g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
	g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	// Render portal to reset stencil values
	PortalMeshes[shape]->Render( &matrix, camera, PlainColour );

	// Restore normal states
	g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	g_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}


// Prototype function below for mutual recursion (functions that call each other)
void RenderPortals( int part, CCamera* camera, int minX, int minY, int maxX, int maxY );

// Check visiblity of a portal (specify entrance or exit side). If it is visible in the given
// screen area and camera, then render the appropriate target partition and all the portals
// within it. Will apply a transform to the camera before rendering the target partition
// allowing the portal to have its entrance and exit in different places.
// Doesn't deal with portals clipped at the near plane => errors when very close to portals
void RenderPortal( TPortalIter itPortal, bool renderInPortal, 
                   CCamera* camera, int minX, int minY, int maxX, int maxY )
{
	// Different set-up depending on whether rendering entrance or exit portal
	CMatrix4x4 matrix;
	CMatrix4x4 transform;
	int targetPartition;
	if (renderInPortal)
	{
		// Matrix for portal
		matrix = (*itPortal)->InMatrix;

		// Matrix to transfrom camera when looking through portal
		transform = InverseAffine( (*itPortal)->InMatrix ) * (*itPortal)->OutMatrix;

		// Partition on other side of portal
		targetPartition = (*itPortal)->OutPartition;
	}
	else
	{
		// Similar for reverse direction
		matrix = (*itPortal)->OutMatrix;
		transform = InverseAffine( (*itPortal)->OutMatrix ) * (*itPortal)->InMatrix;
		targetPartition = (*itPortal)->InPartition;
	}

	// Get world space polygon for the portal
	// Need to transform the portal's shape by the portal's world matrix
	TPortalShape portalPoly;
	TransformPortalShape( (*itPortal)->Shape, matrix, portalPoly );

	// Get vector from portal to camera and facing vector of portal
	CVector3 portalCamera = camera->Position() - portalPoly[0];
	CVector3 portalFacing = GetPortalPolyNormal( portalPoly );
	
	// Only consider portal if it is facing the camera
	if ((renderInPortal && Dot( portalCamera, portalFacing ) > 0.0f) ||
		(!renderInPortal && Dot( portalCamera, portalFacing ) < 0.0f))
	{
		// Convert each portal point to screen coords, keep track of min and max coords
		int portalMinX = ViewportWidth, portalMaxX = -1; // Set initial off-screen values so
		int portalMinY = ViewportHeight, portalMaxY = -1;// portal fails if no pts on screen
		for (int pt = 0; pt < 4; ++pt)
		{
			// Convert portal coordinate to screen pt - if possible
			int x, y;
			if (camera->PixelFromWorldPt( portalPoly[pt], ViewportWidth, ViewportHeight, &x, &y ))
			{
				// Test portal screen point against current min/max
				portalMinX = Min( x, portalMinX );
				portalMinY = Min( y, portalMinY );
				portalMaxX = Max( x, portalMaxX );
				portalMaxY = Max( y, portalMaxY );
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

			// Test target partition of portal
			if (1)//!Partitions[targetPartition].Rendered)
			{
				// Prepare stencil/z-buffer in portal area
				PreRenderPortalShape( (*itPortal)->Shape, matrix, camera );

				// Prepare a custom clipping plane for the portal, this prevents geometry *nearer*
				// than the portal being visible through it (similar to stencil mirror issue).
				// Can only occur on a portal with the entrance & exit in different places
				D3DXPLANE portalPlane, clipPlane;
				D3DXPlaneFromPoints( &portalPlane, ToD3DXVECTORPtr(&portalPoly[0]),
				                     ToD3DXVECTORPtr(&portalPoly[renderInPortal?2:1]),
				                     ToD3DXVECTORPtr(&portalPoly[renderInPortal?1:2]) );
				D3DXMATRIXA16 planeViewProjMatrix = // Extra step needed as per D3DXPlaneTransform documentation...
					ToD3DXMATRIX(Transpose( Inverse( camera->GetViewProjMatrix() ) ));
				D3DXPlaneTransform( &clipPlane, &portalPlane, &planeViewProjMatrix );
				g_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0 );
				g_pd3dDevice->SetClipPlane( 0, (float*)&clipPlane );

				// Transform camera to new position in target partition
				CMatrix4x4 prevCameraMatrix = camera->Matrix();
				camera->Matrix() *= transform;
				camera->CalculateMatrices();

				// Render the new partition
				RenderPartition( targetPartition, camera );

				// Render all portals of new partition if visible in the new screen area
				// (Calls function below, not this one)
				RenderPortals( targetPartition, camera, newMinX, newMinY, newMaxX, newMaxY );

				// Reset camera to previous position (source partition)
				camera->Matrix() = prevCameraMatrix;
				camera->CalculateMatrices();

				// Switch off the custom clip plane
				g_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, 0 );

				// Reset stencil buffer in portal area
				PostRenderPortalShape( (*itPortal)->Shape, matrix, camera );
			}
		}
	}
}


// Check visiblity of portals in a partition - if any are visible in the given screen area
// and camera, then render them, and then recursively render their visible portals
// Doesn't deal with portals clipped at the near plane => errors when very close to portals
// Checks both entrance and exit polygons of each portal, calls function above for main work
void RenderPortals( int part, CCamera* camera, int minX, int minY, int maxX, int maxY )
{
	// Limit recursion through portals
	// e.g Possible to set up a portal whose exit can see its entrance (!)
	if (PortalDepth > 4) return;

	// For each portal of given partition
	TPortalIter itPortal = Partitions[part].Portals.begin();
	while (itPortal != Partitions[part].Portals.end())
	{
		// If entrance portal is in current partition....
		if ((*itPortal)->InPartition == part)
		{
			// Test visibility of entrance portal, and render its target partition (the partition
			// containing the exit) if appropriate
			RenderPortal( itPortal, true, camera, minX, minY, maxX, maxY );
		}

		// Same process for the exit portal...
		if ((*itPortal)->OutPartition == part)
		{
			RenderPortal( itPortal, false, camera, minX, minY, maxX, maxY );
		}

		++itPortal;
	}
}


//-----------------------------------------------------------------------------
// Portal Management Functions
//-----------------------------------------------------------------------------

// Add a portal of the given shape, with given entrance and exit position & Y rotation (radians).
// Will automatically calculate the partitions that will be linked and add the portal to those
// partitions, as well as storing it in the global list of portals.
void AddPortal( int shape, CVector3 inPos, float inRotY, CVector3 outPos, float outRotY )
{
	// Create new portal
	SPortal* newPortal = new SPortal;
	newPortal->Shape = shape;

	// Build world matrices for entrance and exit portals using the positions/rotations provided
	newPortal->InMatrix.MakeAffineEuler( inPos, CVector3(0.0f, inRotY, 0.0f) );
	newPortal->OutMatrix.MakeAffineEuler( outPos, CVector3(0.0f, outRotY, 0.0f) );

	// Automatically determine which partition a portal is in. Get a point by moving inwards 
	// from its position by a small amount (defined here) using the facing vector. Then use the
	// function GetPartitionFromPt on this point.
	const float portalFacingOffset = 0.1f;

	// Determine entrance partition using method described above
	TPortalShape inPortalPoly;
	TransformPortalShape( shape, newPortal->InMatrix, inPortalPoly );
	CVector3 inPortalFacing = GetPortalPolyNormal( inPortalPoly );
	newPortal->InPartition = GetPartitionFromPt( inPos + portalFacingOffset * inPortalFacing );

	// Determine exit partition using method described above
	TPortalShape outPortalPoly;
	TransformPortalShape( shape, newPortal->OutMatrix, outPortalPoly );
	CVector3 outPortalFacing = GetPortalPolyNormal( outPortalPoly );
	newPortal->OutPartition = GetPartitionFromPt( outPos - portalFacingOffset * outPortalFacing );

	// Add partition to global list of portals, and to the partition portal lists
	Portals.push_back( newPortal );
	Partitions[newPortal->InPartition].Portals.push_back( newPortal );
	Partitions[newPortal->OutPartition].Portals.push_back( newPortal );
}

// Release the global list of portals
void RemoveAllPortals()
{
	// For each portal of given partition
	TPortalIter itPortal = Portals.begin();
	while (itPortal != Portals.end())
	{
		delete (*itPortal);
		++itPortal;
	}
}


//-----------------------------------------------------------------------------
// Portal Travel
//-----------------------------------------------------------------------------

// Determine where an moving object will appear including any travel through portals
// Pass an starting world matrix and a vector of movement, returns the finishing world matrix
CMatrix4x4 PortalMove( CMatrix4x4 startMat, CVector3 moveVec )
{
	int part = GetPartitionFromPt( startMat.Position() );

	// For each portal of given partition
	TPortalIter itPortal = Partitions[part].Portals.begin();
	while (itPortal != Partitions[part].Portals.end())
	{
		// Calculate world space portal entrance polygon
		TPortalShape portalPoly;
		TransformPortalShape( (*itPortal)->Shape, (*itPortal)->InMatrix, portalPoly );

		// See if any intersection of movement and the two triangles in the portal polygon
		float u, v, dist;
		if (D3DXIntersectTri( ToD3DXVECTORPtr(&portalPoly[0]), ToD3DXVECTORPtr(&portalPoly[1]),  
		                      ToD3DXVECTORPtr(&portalPoly[2]), ToD3DXVECTORPtr(&startMat.Position()),
                              ToD3DXVECTORPtr(&moveVec), &u, &v, &dist ) ||
		    D3DXIntersectTri( ToD3DXVECTORPtr(&portalPoly[0]), ToD3DXVECTORPtr(&portalPoly[2]),  
		                      ToD3DXVECTORPtr(&portalPoly[3]), ToD3DXVECTORPtr(&startMat.Position()),
                              ToD3DXVECTORPtr(&moveVec), &u, &v, &dist ))
		{
			// If the intersection was within the distance moved
			if (dist < 1.0f)
			{
				// Update matrix to reflect movement and transfomation through portal
				CMatrix4x4 endMat = startMat;
				endMat.Move( moveVec );
				endMat *= InverseAffine( (*itPortal)->InMatrix ) * (*itPortal)->OutMatrix;
				return endMat;
			}
		}


		// Calculate world space portal exit polygon
		TransformPortalShape( (*itPortal)->Shape, (*itPortal)->OutMatrix, portalPoly );

		// See if any intersection of movement and the two triangles in the portal polygon
		if (D3DXIntersectTri( ToD3DXVECTORPtr(&portalPoly[0]), ToD3DXVECTORPtr(&portalPoly[1]),  
		                      ToD3DXVECTORPtr(&portalPoly[2]), ToD3DXVECTORPtr(&startMat.Position()),
                              ToD3DXVECTORPtr(&moveVec), &u, &v, &dist ) ||
		    D3DXIntersectTri( ToD3DXVECTORPtr(&portalPoly[0]), ToD3DXVECTORPtr(&portalPoly[2]),  
		                      ToD3DXVECTORPtr(&portalPoly[3]), ToD3DXVECTORPtr(&startMat.Position()),
                              ToD3DXVECTORPtr(&moveVec), &u, &v, &dist ))
		{
			// If the intersection was within the distance moved
			if (dist < 1.0f)
			{
				// Update matrix to reflect movement and transfomation through portal
				CMatrix4x4 endMat = startMat;
				endMat.Move( moveVec );
				endMat *= InverseAffine( (*itPortal)->OutMatrix ) * (*itPortal)->InMatrix;
				return endMat;
			}
		}

		++itPortal;
	}

	CMatrix4x4 endMat = startMat;
	endMat.Move( moveVec );
	return endMat;
}


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
	Partitions[1].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Door A-G", "Door A-G" );
	Partitions[0].Entities.push_back( id );
	Partitions[6].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Door B-C", "Door B-C" );
	Partitions[1].Entities.push_back( id );
	Partitions[2].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Door C-D", "Door C-D" );
	Partitions[2].Entities.push_back( id );
	Partitions[3].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Door D-E", "Door D-E" );
	Partitions[3].Entities.push_back( id );
	Partitions[4].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Door E-F", "Door E-F" );
	Partitions[4].Entities.push_back( id );
	Partitions[5].Entities.push_back( id );

	id = EntityManager.CreateEntity( "Window A-C", "Window A-C" );
	Partitions[0].Entities.push_back( id );
	Partitions[2].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Window A-D 1", "Window A-D 1" );
	Partitions[0].Entities.push_back( id );
	Partitions[3].Entities.push_back( id );
	id = EntityManager.CreateEntity( "Window A-D 2", "Window A-D 2" );
	Partitions[0].Entities.push_back( id );
	Partitions[3].Entities.push_back( id );

	
	/////////////////////////////
	// Portal Setup

	// Create two meshes for each portal shape - using a combination of two render methods to
	// clear the viewport and depth buffer - allowing portals to "cut holes" in existing geometry
	for (int portal = 0; portal < NumPortalShapes; ++portal)
	{
		PortalMeshes[portal] = new CMesh();
		PortalMeshes[portal]->Create( 4, 12, PortalShapes[portal], 6, PortalIndices, PlainColour );
	}
	// Load additional render methods that will be used for the multi-pass portal rendering
	// See RenderPortalShape for details
	LoadMethod( PlainColour );
	LoadMethod( ClearDepth );


	// Add portals for existing doors / windows. Entrance and exit portals are the same here
	AddPortal( Door, CVector3(-1.5f, 0.0f, -4.75f), 0,   // Entrance   // Door A-B
	                 CVector3(-1.5f, 0.0f, -4.75f), 0 ); // Exit 
	AddPortal( Door, CVector3(-7.975f, 0.0f, -3.25f), ToRadians(90),   // Door A-G
                     CVector3(-7.975f, 0.0f, -3.25f), ToRadians(90) );
	AddPortal( Door, CVector3(-1.5f, 0.0f, -3.15f), 0,                 // Door B-C
                     CVector3(-1.5f, 0.0f, -3.15f), 0 );
	AddPortal( Door, CVector3(-0.5f, 0.0f, -0.05f), 0,                 // Door C-D
                     CVector3(-0.5f, 0.0f, -0.05f), 0 );
	AddPortal( Door, CVector3(3.55f, 0.0f, 2.05f), ToRadians(90),      // Door D-E
                     CVector3(3.55f, 0.0f, 2.05f), ToRadians(90) );
	AddPortal( Door, CVector3(3.55f, 0.0f, -1.45f), ToRadians(90),     // Door E-F
                     CVector3(3.55f, 0.0f, -1.45f), ToRadians(90) );
	AddPortal( Window, CVector3(-3.65f, 0.75f, -1.5f), ToRadians(90),    // Window A-C
                       CVector3(-3.65f, 0.75f, -1.5f), ToRadians(90) );
	AddPortal( Window, CVector3( 1.75f, 0.75f, 4.05f), 0,                // Window A-D 1
                       CVector3( 1.75f, 0.75f, 4.05f), 0 );
	AddPortal( Window, CVector3(-1.75f, 0.75f, 4.05f), 0,                // Window A-D 2
                       CVector3(-1.75f, 0.75f, 4.05f), 0 );

	// Add a new portal with a tapered shape in the middle of a wall (see EPortalShape for shapes)
	AddPortal( Taper, CVector3(2.5f, 0.0f, -3.21f), 0, 
                      CVector3(2.5f, 0.0f, -3.09f), 0 );
	AddPortal(Taper, CVector3(4.5f, 0.0f, -3.21f), 0,
		CVector3(4.5f, 0.0f, -3.09f), 0);

	AddPortal(Door, CVector3(-7.975f, 0.0f, -1.45f), ToRadians(90),
		CVector3(3.58f, 0.0f, -0.00f), ToRadians(90));


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

	// Release global portal list and portal meshes
	RemoveAllPortals();
	for (int portal = 0; portal < NumPortalShapes; ++portal)
	{
		delete PortalMeshes[portal];
	}

	// Destroy all entities / templates
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
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

		// Mark all partitions as not rendered
		for (int part = 0; part < NumPartitions; ++part)
		{
			Partitions[part].Rendered = false;
		}

		// Find current partition
		int currentPartition = GetPartitionFromPt( MainCamera->Position() );

		// Render all entities in the current partition
		RenderPartition( currentPartition, MainCamera );

		PortalDepth = 0;
		g_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, TRUE );
		g_pd3dDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
		g_pd3dDevice->SetRenderState( D3DRS_STENCILREF, PortalDepth );

		// Render partitions visible (in the viewport area) through the portals in
		// the current partition
		RenderPortals( currentPartition, MainCamera, 0, 0, ViewportWidth - 1, ViewportHeight - 1 );

		g_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );

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

	// Move the camera - accumulate movement from keys, then use special portal move function
	CMatrix4x4 camMat = MainCamera->Matrix();
	CVector3 moveVec = CVector3::kZero;
	if (KeyHeld( Key_W ))
	{
		moveVec += camMat.ZAxis() * CameraMoveSpeed * updateTime;
	}
	if (KeyHeld( Key_S ))
	{
		moveVec -= camMat.ZAxis() * CameraMoveSpeed * updateTime;
	}
	if (KeyHeld( Key_A ))
	{
		moveVec -= camMat.XAxis() * CameraMoveSpeed * updateTime;
	}
	if (KeyHeld( Key_D ))
	{
		moveVec += camMat.XAxis() * CameraMoveSpeed * updateTime;
	}
	MainCamera->Matrix() = PortalMove( MainCamera->Matrix(), moveVec );
	MainCamera->CalculateMatrices();

	// Use normal control for rotation
	MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_0, Key_0, Key_0, Key_0, 
	                     CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime );
}


} // namespace gen
