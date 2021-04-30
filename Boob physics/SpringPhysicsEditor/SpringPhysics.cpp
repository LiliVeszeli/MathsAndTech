//-----------------------------------------------------
// SpringPhysics.cpp
//   Point/Spring based physics
//-----------------------------------------------------

#include <list>
#include <fstream>
using namespace std;

#include <direct.h>  // For directory control at start of main
#include <windows.h> // For dialog boxes and other miscellany

#include <TL-Engine.h>
using namespace tle;

#include "CVector3.h"
#include "MathIO.h"
#include "Particle.h"
#include "Spring.h"
#include "Support.h"
using namespace gen;


//------------------------------------------
// Spring system
//------------------------------------------

const float    DEFAULT_MASS        = 1.0f;
const float    DEFAULT_COEFFICIENT = 14.0f;
const CVector3 GRAVITY             = CVector3(0, -50.0f, 0);

list<CParticle*> Particles;
list<CSpring*> Springs;

bool Simulating = false;


//------------------------------------------
// Engine / Camera
//------------------------------------------

// Global engine elements
I3DEngine* Engine;
ICamera* Camera;
float NearClip = 0.1f;
float CameraRotX = 0;
float CameraRotY = 0;

// Height of floor (and shadows)
float FloorHeight = 0;

// Engine / camera constants
const float MONITOR_REFRESH_RATE = 60.0f; // In Hertz
const float CAMERA_MOVE_SPEED    = 25.0f; // In units / second
const float CAMERA_ROT_SPEED     = 0.25f; // In degrees / pixel mouse movement
const float CAMERA_ROT_SMOOTH    = 0.15f; // Approx num seconds of rotational deceleration
const int   WINDOW_MOUSE_BORDER  = 40;    // Size of border at edge of window in which mouse rotates camera
const float BORDER_ROT_SPEED     = 270;   // Degrees / second of camera rotation when mouse in window border
const float FLOOR_MOVE_SPEED     = 5.0f;  // Units / second


//------------------------------------------
// UI
//------------------------------------------

// UI states and data
enum UIState
{
	EditMode,
	FreeMove,
	PlaceParticle,
	PlaceSpringStart,
	PlaceSpringEnd,
	SpringSettings,
};
UIState State = EditMode;

// UI/physics system meshes 
IMesh* SelectionMesh;
IMesh* ParticleMesh;
IMesh* ShadowMesh;
IMesh* SpringMesh;
IMesh* InertialMesh;
IMesh* SkinMesh;
IModel* SkinModel;

// UI constants
const float DEFAULT_DISTANCE        = 80.0f;  // Inital distance from camera of placed particle
const float DISTANCE_CHANGE_SPEED   = 10.0f;  // Units / second
const float MASS_CHANGE_SPEED       = 0.04f;  // Percent / 100 per mouse wheel tick
const float LENGTH_CHANGE_SPEED     = 0.2f;   // Units per mouse wheel tick
const float COEFFICENT_CHANGE_SPEED = 100.0f; // Units / second
const float MIN_COEFFICENT          = 0.05f;

// Particle / spring being edited
CParticle* EditParticle;
CSpring* EditSpring;
list<CParticle*>::iterator EditParticleIter; // Set when editing an existant particle, otherwise Particles->end()
list<CSpring*>::iterator EditSpringIter;     // Similar
float ZDistance = DEFAULT_DISTANCE;          // From camera
float CurrMass = DEFAULT_MASS;



//------------------------------------------
// Load / Save
//------------------------------------------

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

// Get pointer to spring with given UID or NULL if no such spring
CSpring* SpringFromUID( unsigned int UID )
{
	list<CSpring*>::iterator itSpring = Springs.begin();
	while (itSpring != Springs.end())
	{
		if ((*itSpring)->GetUID() == UID) return (*itSpring);
		itSpring++;
	}
	return NULL;
}

// Remove all particles and springs
void NewSystem()
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

// Save system to a file - will prompt for filename. Returns true on success
bool SaveSystem()
{
	// Old-school Windows programming to get file save as dialog. Don't use this style in a forms-based application!
	char fileName[MAX_PATH] = "";
	OPENFILENAME openFile;
	ZeroMemory( &openFile, sizeof(OPENFILENAME) );
	openFile.lStructSize = sizeof(OPENFILENAME);
    openFile.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    openFile.lpstrFilter = "Particle Physics Files (*.ptf)\0*.ptf\0All Files (*.*)\0*.*\0";
    openFile.lpstrDefExt = "ptf";
    openFile.lpstrFile = fileName;
    openFile.nMaxFile = MAX_PATH;

	// Open file as a text stream
	if (!GetSaveFileName(&openFile)) return false;
	ofstream file( fileName, ios::out | ios::trunc );
	if (!file) return false;

	// Output key information of each particle and spring. Save UIDs instead of pointers
	file << "PARTICLES" << endl;
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		file << "P " << (*itParticle)->GetPosition() << " " << (*itParticle)->GetMass() << " " << (*itParticle)->IsPinned() << " " << (*itParticle)->GetUID() << endl;
		itParticle++;
	}
	file << "SPRINGS" << endl;
	list<CSpring*>::iterator itSpring = Springs.begin();
	while (itSpring != Springs.end())
	{
		file << "S " << (*itSpring)->GetParticle1()->GetUID() << " " << (*itSpring)->GetParticle2()->GetUID() << " " << (*itSpring)->GetCoefficient()
		     << " " << (*itSpring)->GetInertialLength() << " " << (*itSpring)->GetType() << " " << (*itSpring)->GetUID() << endl;
		itSpring++;
	}

	file.close();
	return !file.fail();
}

// Load system from a file - will prompt for filename. Removes current system. Returns true on success
bool LoadSystem()
{
	// Old-school Windows programming to get file open dialog. Don't use this style in a forms-based application!
	char fileName[MAX_PATH] = "";
	OPENFILENAME openFile;
	ZeroMemory( &openFile, sizeof(OPENFILENAME) );
	openFile.lStructSize = sizeof(OPENFILENAME);
    openFile.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    openFile.lpstrFilter = "Particle Physics Files (*.ptf)\0*.ptf\0All Files (*.*)\0*.*\0";
    openFile.lpstrDefExt = "ptf";
    openFile.lpstrFile = fileName;
    openFile.nMaxFile = MAX_PATH;
	
	// Open file as a text stream
	if (!GetOpenFileName(&openFile)) return false;
	ifstream file( fileName, ios::in );
	if (!file) return false;

	// Clear existing system
	NewSystem();

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
			EditParticle = new CParticle( ParticleMesh, ShadowMesh, position, mass, isPinned, UID );
			Particles.push_back( EditParticle );
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
			CParticle* particle1 = ParticleFromUID(particleUID1);
			CParticle* particle2 = ParticleFromUID(particleUID2);
			if (particle1 == NULL || particle2 == NULL)
			{
				file.setstate( ios::failbit );
			}
			else
			{
				EditSpring = new CSpring( SpringMesh, InertialMesh, particle1, particle2, coefficient, inertialLength,
										  static_cast<CSpring::ESpringType>(springType), UID );
				Springs.push_back( EditSpring );
				particle1->AddSpring( EditSpring );
				particle2->AddSpring( EditSpring );
			}
		}
	}

	file.close();
	if (file.fail())
	{
		NewSystem();
		MessageBox( (HWND)Engine->GetWindow(), "Error loading particle system file", NULL, MB_OK | MB_ICONERROR );
		return false;
	}
	return true;
}

// Load mesh and create model to be skinned - use as template for creating particle system. Returns true on success
bool LoadSkinMesh()
{
	// Old-school Windows programming to get file open dialog. Don't use this style in a forms-based application!
	char fileName[MAX_PATH] = "";
	OPENFILENAME openFile;
	ZeroMemory( &openFile, sizeof(OPENFILENAME) );
	openFile.lStructSize = sizeof(OPENFILENAME);
    openFile.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    openFile.lpstrFilter = "X-file meshes (*.x)\0*.x\0All Files (*.*)\0*.*\0";
    openFile.lpstrDefExt = "x";
    openFile.lpstrFile = fileName;
    openFile.nMaxFile = MAX_PATH;
	
	// Read mesh from file
	if (!GetOpenFileName(&openFile)) return false;
	if (SkinMesh != NULL)
	{
		Engine->RemoveMesh( SkinMesh );
	}
	SkinMesh = Engine->LoadMesh( fileName );
	if (!SkinMesh)
	{
		MessageBox( (HWND)Engine->GetWindow(), "Error loading mesh", NULL, MB_OK | MB_ICONERROR );
		return false;
	}
	SkinModel = SkinMesh->CreateModel();
	return true;
}


//------------------------------------------
// UI functions
//------------------------------------------

// Get global camera position as a CVector3
CVector3 CameraPosition()
{
	return CVector3( Camera->GetX(), Camera->GetY(), Camera->GetZ() );
}

// Get global camera facing direction as a CVector3
CVector3 CameraFacing()
{
	float cameraMatrixElts[16];
	Camera->GetMatrix( cameraMatrixElts );
	return CVector3( cameraMatrixElts[8], cameraMatrixElts[9], cameraMatrixElts[10] );
}

// Return the world point under the mouse with the given z-distance from the camera
CVector3 PointFromMouse( float cameraZDist )
{
	CVector3 mousePoint = WorldPointFromPixel( Engine->GetMouseX(), Engine->GetMouseY(), Camera, NearClip, Engine );
	CVector3 mouseRay = Normalise( mousePoint - CameraPosition() );
	float rayDistance = cameraZDist / Dot( mouseRay, CameraFacing() );
	return CameraPosition() + mouseRay * rayDistance;
}


// Move and rotate camera, depending on UI state
void UpdateCamera( float updateTime )
{
	// Camera movement
	if (Engine->KeyHeld(Key_W)) Camera->MoveLocalZ( CAMERA_MOVE_SPEED * updateTime );
	if (Engine->KeyHeld(Key_S)) Camera->MoveLocalZ( -CAMERA_MOVE_SPEED * updateTime );
	if (Engine->KeyHeld(Key_D)) Camera->MoveLocalX( CAMERA_MOVE_SPEED * updateTime );
	if (Engine->KeyHeld(Key_A)) Camera->MoveLocalX( -CAMERA_MOVE_SPEED * updateTime );
	if (Engine->KeyHeld(Key_Q)) Camera->MoveLocalY( CAMERA_MOVE_SPEED * updateTime );
	if (Engine->KeyHeld(Key_E)) Camera->MoveLocalY( -CAMERA_MOVE_SPEED * updateTime );

	// Camera rotation depends on state
	float newRotX = 0;
	float newRotY = 0;
	if (Engine->IsActive()) // Ignore mouse rotation if window is not active (e.g. not the foreground window)
	{
		switch (State)
		{
		// Mouse rotates camera directly
		case FreeMove:
			newRotX = static_cast<float>(Engine->GetMouseMovementY());
			newRotY = static_cast<float>(Engine->GetMouseMovementX());
			break;

		// Camera rotates when mouse enters border of window
		case PlaceParticle:
		case PlaceSpringStart:
		case PlaceSpringEnd:
		case EditMode:
			if (Engine->GetMouseX() >= 0 && Engine->GetMouseX() < Engine->GetWidth() && Engine->GetMouseY() >= 0 && Engine->GetMouseY() < Engine->GetHeight())
			{
				if (Engine->GetMouseX() > Engine->GetWidth() - WINDOW_MOUSE_BORDER)
				{
					newRotY = BORDER_ROT_SPEED * updateTime;
				}
				else if (Engine->GetMouseX() < WINDOW_MOUSE_BORDER)
				{
					newRotY = -BORDER_ROT_SPEED * updateTime;
				}
				if (Engine->GetMouseY() > Engine->GetHeight() - WINDOW_MOUSE_BORDER)
				{
					newRotX = BORDER_ROT_SPEED * updateTime;
				}
				else if (Engine->GetMouseY() < WINDOW_MOUSE_BORDER)
				{
					newRotX = -BORDER_ROT_SPEED * updateTime;
				}
			}
			if (Engine->KeyHeld(Key_Right))
			{
				newRotY = BORDER_ROT_SPEED * updateTime;
			}
			else if (Engine->KeyHeld(Key_Left))
			{
				newRotY = -BORDER_ROT_SPEED * updateTime;
			}
			if (Engine->KeyHeld(Key_Down))
			{
				newRotX = BORDER_ROT_SPEED * updateTime;
			}
			else if (Engine->KeyHeld(Key_Up))
			{
				newRotX = -BORDER_ROT_SPEED * updateTime;
			}
			break;
		}
	}

	// Smoothed camera rotation
	float cameraRotSmooth = updateTime / (CAMERA_ROT_SMOOTH * 0.25f);
	CameraRotX = CameraRotX * (1 - cameraRotSmooth) + newRotX * cameraRotSmooth;
	CameraRotY = CameraRotY * (1 - cameraRotSmooth) + newRotY * cameraRotSmooth;
	Camera->RotateLocalX( CameraRotX * CAMERA_ROT_SPEED );
	Camera->RotateY( CameraRotY * CAMERA_ROT_SPEED );
}


//------------------------------------------
// Picking functions
//------------------------------------------

// Returns an iterator to the particle nearest to the mouse (in 2D), also returns the nearest distance via a pointer parameter
// Returns iterator to end of list if it is empty. 
list<CParticle*>::iterator PickParticle( int* nearestDist )
{
	list<CParticle*>::iterator  itPickParticle = Particles.end();

	int mx = Engine->GetMouseX();
	int my = Engine->GetMouseY();

	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		int px, py;
		if (PixelFromWorldPoint( (*itParticle)->GetPosition(), Camera, Engine, &px, &py )) // Returns false if behind camera
		{
			int dist = (px-mx)*(px-mx) + (py-my)*(py-my);
			if (itPickParticle == Particles.end() || dist < *nearestDist)
			{
				*nearestDist = dist;
				itPickParticle = itParticle;
			}
		}
		itParticle++;
	}

	return itPickParticle;
}

// Returns an iterator to the spring nearest to the mouse (in 2D), also returns the nearest distance via a pointer parameter
// Returns iterator to end of list if it is empty. The centre of the spring is targeted
list<CSpring*>::iterator PickSpring( int* nearestDist )
{
	list<CSpring*>::iterator  itPickSpring = Springs.end();

	int mx = Engine->GetMouseX();
	int my = Engine->GetMouseY();

	list<CSpring*>::iterator itSpring = Springs.begin();
	while (itSpring != Springs.end())
	{
		if ((*itSpring)->GetParticle1() && (*itSpring)->GetParticle2())
		{
			CVector3 centrePt = ((*itSpring)->GetParticle1()->GetPosition() + (*itSpring)->GetParticle2()->GetPosition()) * 0.5f;
			int px, py;
			if (PixelFromWorldPoint( centrePt, Camera, Engine, &px, &py )) // Returns false if behind camera
			{
				int dist = (px-mx)*(px-mx) + (py-my)*(py-my);
				if (itPickSpring == Springs.end() || dist < *nearestDist)
				{
					*nearestDist = dist;
					itPickSpring = itSpring;
				}
			}
		}
		itSpring++;
	}

	return itPickSpring;
}


//------------------------------------------
// Simulation Control
//------------------------------------------

void StartSimulation()
{
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		(*itParticle)->InitSimulation();
		++itParticle;
	}
	Simulating = true;
}

void EndSimulation()
{
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		(*itParticle)->ResetSimulation();
		++itParticle;
	}
	Simulating = false;
}

void UpdateSimulation( float updateTime )
{
	// Update particle positions based on forces from springs and external forces (e.g. gravity). Ignore constraints for now
	list<CParticle*>::iterator itParticle = Particles.begin();
	while (itParticle != Particles.end())
	{
		CVector3 externalForces = (*itParticle)->GetMass() * GRAVITY; // Particle mass * gravity acceleration = gravitation force on this particle
		(*itParticle)->ApplyForces( updateTime, externalForces );
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


//------------------------------------------
// Main function
//------------------------------------------

void main()
{
	// TL-Engine setup
	Engine = New3DEngine( kTLX );
	Engine->StartWindowed(1024, 768);
	Engine->Timer();

	// Add current working folder (e.g. executable location) as TL-Engine media folder. Only need this because file dialogs can change working folder
	char currDir[MAX_PATH];
	_getcwd( currDir, MAX_PATH );
	Engine->AddMediaFolder( currDir );

	// Generic scene setup
	Camera = Engine->CreateCamera( kManual, 0, CAMERA_MOVE_SPEED, -DEFAULT_DISTANCE );
	Camera->SetNearClip( NearClip );
	IMesh* floorMesh = Engine->LoadMesh( "Floor.x" );
	IModel* floor = floorMesh->CreateModel(0, FloorHeight - 0.01f, 0); // Slightly downwards to stop z-fighting shadows
	/*IMesh* womanMesh = Engine->LoadMesh("Woman1.x");
	IModel* woman = womanMesh->CreateModel();
	woman->SetSkin("Woman.ptf");*/
	SelectionMesh = Engine->LoadMesh( "Cube.x" );
	ParticleMesh = Engine->LoadMesh( "Particle.x" );
	ShadowMesh = Engine->LoadMesh( "Shadow.x" );
	SpringMesh = Engine->LoadMesh( "Spring.x" );
	InertialMesh = Engine->LoadMesh( "Inertial.x" );

	// Game loop
	while (Engine->IsRunning())
	{
		Engine->DrawScene();

		// Game loop timing. Wait for next monitor refresh (roughly) - approximation to vertical synchronisation
		float updateTime = Engine->Timer();
		while (updateTime < 1 / MONITOR_REFRESH_RATE) updateTime += Engine->Timer();

		// Move / rotate camera
		UpdateCamera( updateTime );

		// Update simulation
		if (Simulating)	UpdateSimulation( updateTime );
		
		// Check for quit
		if(Engine->KeyHit( Key_Escape ))
		{
			Engine->StopMouseCapture();
			if (MessageBox( (HWND)Engine->GetWindow(), "Are you sure you want to quit?", "Confirm", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES)
			{
				break; // Break out of game loop
			}
			if (State == FreeMove)
			{
				Engine->StartMouseCapture();
				Engine->IsRunning();
				Engine->GetMouseMovementX(); // Flush mouse movement - stops camera from jumping when switching to direct mouse movement
				Engine->GetMouseMovementY();
			}
			Engine->Timer(); // Update timer as we will have spent a long time waiting in dialog box
			PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_Escape, 0 ); // Send own key-release message as actual key-up is lost to TL during dialog box
		}

		// Move floor (and redraw particles to move their shadows)
		if (Engine->KeyHeld( Key_Home ))
		{
			FloorHeight += FLOOR_MOVE_SPEED * updateTime;
			floor->SetY( FloorHeight - 0.01f );
			list<CParticle*>::iterator itParticle = Particles.begin();
			while (itParticle != Particles.end())
			{
				(*itParticle)->SetPosition((*itParticle)->GetPosition());
				itParticle++;
			}
		}
		if (Engine->KeyHeld( Key_End ))
		{
			FloorHeight -= FLOOR_MOVE_SPEED * updateTime;
			floor->SetY( FloorHeight - 0.01f );
			list<CParticle*>::iterator itParticle = Particles.begin();
			while (itParticle != Particles.end())
			{
				(*itParticle)->SetPosition((*itParticle)->GetPosition());
				itParticle++;
			}
		}

		// Input depending on UI state
		switch (State)
		{
			//----------------------------------------------------------------------------------------
			// Edit mode, can place items, edit existing ones or switch to free mode + new/save/load
			case EditMode:
			{
				// Find nearest particle or spring to mouse
				int nearestParticle, nearestSpring;
				EditParticleIter = PickParticle( &nearestParticle );
				EditSpringIter = PickSpring( &nearestSpring );

				// Edit nearest of pair on left click
				if (EditParticleIter != Particles.end() && (EditSpringIter == Springs.end() || nearestParticle < nearestSpring))
				{
					if (Engine->KeyHit( Mouse_LButton ))
					{
						EditParticle = *EditParticleIter;
						ZDistance = Dot( EditParticle->GetPosition() - CameraPosition(), CameraFacing() );
						State = PlaceParticle;
						break;
					}
				}
				else if (EditSpringIter != Springs.end())
				{
					if (Engine->KeyHit( Mouse_LButton ))
					{
						EditSpring = *EditSpringIter;
						// Go to last editing state for spring type
						if (EditSpring->GetType() == CSpring::Rod)
						{
							if (!Simulating)
							{
								EditSpring->GetParticle2()->RemoveSpring( EditSpring );
								EditSpring->SetParticle2( NULL );
								State = PlaceSpringEnd;
							}
						}
						else
						{
							EditSpring->ShowInertialModel( true );
							State = SpringSettings;
						}
						break;
					}
				}

				// Place particle (can't while simulating)
				if (!Simulating && Engine->KeyHit( Key_1 ))
				{
					// Create particle at mouse position
					CVector3 particlePt = PointFromMouse( ZDistance );
					EditParticle = new CParticle( ParticleMesh, ShadowMesh, particlePt, CurrMass, false );
					EditParticleIter = Particles.end();
					State = PlaceParticle;
					break;
				}

				// Place spring (can't while simulating)
				if (!Simulating && Engine->KeyHit( Key_2 ))
				{

					// Create particle at mouse position
					CVector3 tempTarget = PointFromMouse( ZDistance );
					EditSpring = new CSpring( SpringMesh, InertialMesh, NULL, NULL, DEFAULT_COEFFICIENT );
					EditSpring->SetTempTarget( tempTarget );
					EditSpringIter = Springs.end();
					State = PlaceSpringStart;
					break;
				}

				// Switch to free move
				if (Engine->KeyHit( Key_Space ))
				{
					Engine->StartMouseCapture();
					Engine->GetMouseMovementX(); // Flush mouse movement - stops camera from jumping when switching to direct mouse movement
					Engine->GetMouseMovementY();
					State = FreeMove;
					break;
				}

				// Toggle simulation
				if (Engine->KeyHit( Key_Return ))
				{
					if (!Simulating) StartSimulation();
					else EndSimulation();
					break;
				}

				// New
				if (Engine->KeyHit( Key_F1 ))
				{
					if (MessageBox( (HWND)Engine->GetWindow(), "Are you sure you want to start a new particle system?", "Confirm", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES)
					{
						if (Simulating) EndSimulation();
						NewSystem();
					}
					Engine->Timer(); // Update timer as we will have spent a long time waiting in dialog box
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F1, 0 ); // Send own key-release message as actual key-up is lost to TL during dialog box
				}
				// Save
				if (Engine->KeyHit( Key_F5 ))
				{
					if (Simulating) EndSimulation();
					SaveSystem();
					Engine->Timer(); // --"--
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F5, 0 ); //--"--
				}
				// Load
				if (Engine->KeyHit( Key_F9 ))
				{
					if (Simulating) EndSimulation();
					LoadSystem();
					Engine->Timer(); // --"--
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F9, 0 ); //--"--
				}
				// Load mesh to be skinned - use as template
				if (Engine->KeyHit( Key_F7 ))
				{
					LoadSkinMesh();
					Engine->Timer(); // --"--
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F7, 0 ); //--"--
				}
				// Remove mesh to be skinned
				if (Engine->KeyHit( Key_F8 ) && SkinMesh != NULL)
				{
					Engine->RemoveMesh( SkinMesh );
					SkinMesh = NULL;
				}
			}
			break;

			//-------------------------------------------------------------------------------------
			// Free moving camera, can place items or switch to edit mode + new/save/load
			case FreeMove:
			{
				// Place particle (can't while simulating)
				if (!Simulating && Engine->KeyHit( Key_1 ))
				{
					// Create particle at mouse position
					CVector3 particlePt = PointFromMouse( ZDistance );
					EditParticle = new CParticle( ParticleMesh, ShadowMesh, particlePt, CurrMass, false );
					EditParticleIter = Particles.end();
					Engine->StopMouseCapture();
					State = PlaceParticle;
					break;
				}

				// Place spring (can't while simulating)
				if (!Simulating && Engine->KeyHit( Key_2 ))
				{
					// Create particle at mouse position
					CVector3 tempTarget = PointFromMouse( ZDistance );
					EditSpring = new CSpring( SpringMesh, InertialMesh, NULL, NULL, DEFAULT_COEFFICIENT );
					EditSpring->SetTempTarget( tempTarget );
					EditSpringIter = Springs.end();
					Engine->StopMouseCapture();
					State = PlaceSpringStart;
					break;
				}

				// Edit mode
				if (Engine->KeyHit( Key_Space ))
				{
					Engine->StopMouseCapture();
					State = EditMode;
					break;
				}

				// Toggle simulation
				if (Engine->KeyHit( Key_Return ))
				{
					if (!Simulating) StartSimulation();
					else EndSimulation();
					break;
				}

				// New
				if (Engine->KeyHit( Key_F1 ))
				{
					Engine->StopMouseCapture();
					if (MessageBox( (HWND)Engine->GetWindow(), "Are you sure you want to start a new particle system?", "Confirm", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES)
					{
						if (Simulating) EndSimulation();
						NewSystem();
					}
					Engine->StartMouseCapture();
					Engine->IsRunning();
					Engine->GetMouseMovementX();
					Engine->GetMouseMovementY();
					Engine->Timer(); // Update timer as we will have spent a long time waiting in dialog box
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F1, 0 ); // Send own key-release message as actual key-up is lost to TL during dialog box
				}
				// Save
				if (Engine->KeyHit( Key_F5 ))
				{
					Engine->StopMouseCapture();
					if (Simulating) EndSimulation();
					SaveSystem();
					Engine->StartMouseCapture();
					Engine->IsRunning();
					Engine->GetMouseMovementX();
					Engine->GetMouseMovementY();
					Engine->Timer(); // --"--
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F5, 0 ); //--"--
				}
				// Load
				if (Engine->KeyHit( Key_F9 ))
				{
					Engine->StopMouseCapture();
					if (Simulating) EndSimulation();
					LoadSystem();
					Engine->StartMouseCapture();
					Engine->IsRunning();
					Engine->GetMouseMovementX();
					Engine->GetMouseMovementY();
					Engine->Timer(); // --"--
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F9, 0 ); //--"--
				}
				// Load mesh to be skinned - use as template
				if (Engine->KeyHit( Key_F7 ))
				{
					Engine->StopMouseCapture();
					LoadSkinMesh();
					Engine->StartMouseCapture();
					Engine->IsRunning();
					Engine->GetMouseMovementX();
					Engine->GetMouseMovementY();
					Engine->Timer(); // --"--
					PostMessage( (HWND)Engine->GetWindow(), WM_KEYUP, Key_F7, 0 ); //--"--
				}
				// Remove mesh to be skinned
				if (Engine->KeyHit( Key_F8 ) && SkinMesh != NULL)
				{
					Engine->RemoveMesh( SkinMesh );
					SkinMesh = NULL;
				}
			}
			break;

			//-------------------------------------------------------------------------------------
			// Placing a particle: add/move particle or cancel
			case PlaceParticle:
			{
				// Move particle nearer or further
				if (Engine->KeyHeld( Key_R )) ZDistance += DISTANCE_CHANGE_SPEED * updateTime;
				if (Engine->KeyHeld( Key_F )) ZDistance -= DISTANCE_CHANGE_SPEED * updateTime;

				// Change mass on mouse wheel
				CurrMass = EditParticle->GetMass() * (1.0f + MASS_CHANGE_SPEED * Engine->GetMouseWheelMovement());
				EditParticle->SetMass( CurrMass );

				// Toggle pinning
				if (Engine->KeyHit( Key_Space ))
				{
					EditParticle->Pin( !EditParticle->IsPinned() );
				}

				// Update position of particle/shadow being placed (follows mouse). Keep track of movement for code below
				CVector3 oldPosition = EditParticle->GetPosition();
				EditParticle->SetPosition( PointFromMouse( ZDistance ) );
				CVector3 movement = EditParticle->GetPosition() - oldPosition;

				// Move all other particles by same amount if X is pressed, or just particles with the same pinning if Z is pressed (otherwise only the one particle moves)
				bool heldX = Engine->KeyHeld( Key_X );
				bool heldZ = Engine->KeyHeld( Key_Z );
				if (heldX || heldZ)
				{
					list<CParticle*>::iterator itParticle = Particles.begin();
					while (itParticle != Particles.end())
					{
						if ((heldX || EditParticle->IsPinned() == (*itParticle)->IsPinned()) && (*itParticle) != EditParticle)
						{
							(*itParticle)->SetPosition( (*itParticle)->GetPosition() + movement );
						}
						itParticle++;
					}
				}


				// When editing but *not* simulating - force inertial length of attached rods / strings to be equal to distance between particles
				// If you want an initially loose string, need to manually edit string after particle placement
				if (!Simulating)
				{
					list<CSpring*>::iterator itSpring = EditParticle->GetSprings().begin();
					while (itSpring != EditParticle->GetSprings().end())
					{
						if ((*itSpring)->GetType() == CSpring::Rod || (*itSpring)->GetType() == CSpring::String)
						{
							float particleDist = Distance((*itSpring)->GetParticle1()->GetPosition(), (*itSpring)->GetParticle2()->GetPosition());
							(*itSpring)->SetInertialLength(particleDist);
						}
						itSpring++;
					}
				}

				// Cancel placement / delete particle (can't while simulating)
				if (!Simulating && Engine->KeyHit( Mouse_RButton ))
				{
					if (EditParticleIter != Particles.end())
					{
						// Remove particle and any attached springs
						Particles.erase( EditParticleIter );
						list<CSpring*>::iterator itSpring = Springs.begin();
						while (itSpring != Springs.end())
						{
							if ((*itSpring)->GetParticle1() == EditParticle || (*itSpring)->GetParticle2() == EditParticle)
							{
								delete (*itSpring);
								itSpring = Springs.erase( itSpring );
							}
							else
							{
								itSpring++;
							}
						}
					}
					delete EditParticle;
					State = EditMode;
					break;
				}

				// Placement complete
				if (Engine->KeyHit( Mouse_LButton ))
				{
					if (EditParticleIter == Particles.end())
					{
						Particles.push_back( EditParticle );
					}
					State = EditMode;
					break;
				}

				// Toggle simulation
				if (Engine->KeyHit( Key_Return ))
				{
					if (!Simulating) StartSimulation();
					else EndSimulation();
					break;
				}
			}
			break;

			//-------------------------------------------------------------------------------------
			// Placing a spring (start particle): attach, cancel, switch type
			case PlaceSpringStart:
			{
				// Update temporary target of spring being placed (follows mouse)
				EditSpring->SetTempTarget( PointFromMouse( ZDistance ) );

				// Cancel placement / delete spring
				if (Engine->KeyHit( Mouse_RButton ))
				{
					if (EditSpringIter != Springs.end())
					{
						Springs.erase( EditSpringIter );
					}
					delete EditSpring;
					State = EditMode;
					break;
				}

				// Find nearest particle to mouse
				int nearestDist;
				list<CParticle*>::iterator itPickParticle = PickParticle( &nearestDist );

				// Attach to it on left click
				if (itPickParticle != Particles.end() && Engine->KeyHit( Mouse_LButton ))
				{
					(*itPickParticle)->AddSpring( EditSpring );
					EditSpring->SetParticle1( *itPickParticle );
					State = PlaceSpringEnd;
					break;
				}

				// Change type on mouse wheel
				float wheel = Engine->GetMouseWheelMovement();
				unsigned int type = static_cast<unsigned int>(EditSpring->GetType());
				if (wheel > 0.0f)
				{
					type++;
					if (type == CSpring::NumTypes) type = 0;
					EditSpring->SetType( static_cast<CSpring::ESpringType>(type) );
				}
				else if (wheel < 0.0f)
				{
					if (type == 0) type = CSpring::NumTypes;
					type--;
					EditSpring->SetType( static_cast<CSpring::ESpringType>(type) );
				}
			}
			break;

			//-------------------------------------------------------------------------------------
			// Placing a spring (end particle): attach, cancel, switch type
			case PlaceSpringEnd:
			{
				// Update temporary target of spring being placed (follows mouse)
				EditSpring->SetTempTarget( PointFromMouse( ZDistance ) );

				// Cancel placement - go back to editing start particle
				if (Engine->KeyHit( Mouse_RButton ))
				{
					EditSpring->GetParticle1()->RemoveSpring( EditSpring );
					EditSpring->SetParticle1( NULL );
					State = PlaceSpringStart;
					break;
				}

				// Find nearest particle to mouse
				int nearestDist;
				list<CParticle*>::iterator itPickParticle = PickParticle( &nearestDist );

				// Attach to it on left click
				if (itPickParticle != Particles.end() && *itPickParticle != EditSpring->GetParticle1() && Engine->KeyHit( Mouse_LButton ))
				{
					(*itPickParticle)->AddSpring( EditSpring );
					EditSpring->SetParticle2( *itPickParticle );
					EditSpring->SetInertialLength( 0.0f );

					// Rods skip the setting of inertial length - just add to list
					if (EditSpring->GetType() == CSpring::Rod)
					{
						if (EditSpringIter == Springs.end())
						{
							Springs.push_back( EditSpring );
						}
						State = EditMode;
					}
					else
					{
						EditSpring->ShowInertialModel( true );
						State = SpringSettings;
					}
					break;
				}

				// Change type on mouse wheel
				float wheel = Engine->GetMouseWheelMovement();
				unsigned int type = static_cast<unsigned int>(EditSpring->GetType());
				if (wheel > 0.0f)
				{
					type++;
					if (type == CSpring::NumTypes) type = 0;
					EditSpring->SetType( static_cast<CSpring::ESpringType>(type) );
				}
				else if (wheel < 0.0f)
				{
					if (type == 0) type = CSpring::NumTypes;
					type--;
					EditSpring->SetType( static_cast<CSpring::ESpringType>(type) );
				}
			}
			break;

			//-------------------------------------------------------------------------------------
			// Set spring length / coefficient: set length, confirm, cancel
			case SpringSettings:
			{
				// Cancel placement - go back to editing end particle
				if (!Simulating && Engine->KeyHit( Mouse_RButton ))
				{
					EditSpring->GetParticle2()->RemoveSpring( EditSpring );
					EditSpring->SetParticle2( NULL );
					EditSpring->SetCoefficient( DEFAULT_COEFFICIENT );
					EditSpring->ShowInertialModel( false );
					State = PlaceSpringEnd;
					break;
				}

				// Finalise spring on left click
				if (Engine->KeyHit( Mouse_LButton ))
				{
					if (EditSpringIter == Springs.end())
					{
						Springs.push_back( EditSpring );
					}
					EditSpring->ShowInertialModel( false );
					State = EditMode;
					break;
				}

				// Change inertial length on mouse wheel
				float newLength = EditSpring->GetInertialLength() + LENGTH_CHANGE_SPEED * Engine->GetMouseWheelMovement();

				// Set lower limit on inertial length and stop strings having a shorter length than distance between particles
				if (newLength < LENGTH_CHANGE_SPEED * 2)
				{
					newLength = LENGTH_CHANGE_SPEED * 2;
				}
				if (EditSpring->GetType() == CSpring::String && 
					newLength < Distance(EditSpring->GetParticle1()->GetPosition(), EditSpring->GetParticle2()->GetPosition()))
				{
					newLength = Distance(EditSpring->GetParticle1()->GetPosition(), EditSpring->GetParticle2()->GetPosition());
				}
				EditSpring->SetInertialLength( newLength );

				// Alter spring coefficient (not strings)
				if (EditSpring->GetType() != CSpring::String)
				{
					float newCoeff = EditSpring->GetCoefficient();
					if (Engine->KeyHeld( Key_R )) newCoeff += COEFFICENT_CHANGE_SPEED * updateTime;
					if (Engine->KeyHeld( Key_F )) newCoeff -= COEFFICENT_CHANGE_SPEED * updateTime;
					if (newCoeff < MIN_COEFFICENT) newCoeff = MIN_COEFFICENT;
					EditSpring->SetCoefficient( newCoeff );
				}

				// Toggle simulation
				if (Engine->KeyHit( Key_Return ))
				{
					if (!Simulating) StartSimulation();
					else EndSimulation();
					break;
				}
			}
			break;
		}
	}

	// Clear up particle/spring lists
	NewSystem();

	// Delete the 3D engine now we are finished with it
	Engine->Delete();
}
