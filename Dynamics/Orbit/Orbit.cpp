// Orbit.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
using namespace tle;

#include <CVector3.h>
using namespace gen;

// TODO: Given object's position and velocity, return acceleration needed to stay in orbit around a centre point
// Acceleration will be s^2/r in the direction of the centre, where s is object's speed and r is the radius of orbit
CVector3 OrbitAcceleration( CVector3& position, float speed, CVector3& centre )
{
	CVector3 accel;

	// Speed is given in a parameter. Radius can be calculated from centre->position distance. Then use formula to calculate **amount** of acceleration (length of acceleration vector)
	// Direction of acceleration is from position to centre - normalise this vector and multiply by the amount you just calculated. Return that result in accel

	return accel;
}

// TODO: Update position and velocity using Euler method so it orbits a centre point
void OrbitEulerMethod( CVector3& position, CVector3& velocity, CVector3& centre, float updateTime )
{
	CVector3 accel = OrbitAcceleration( position, Length(velocity), centre );
	//...
}


void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine( kTLX );
	myEngine->StartWindowed();
	myEngine->Timer();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder( "C:\\ProgramData\\TL-Engine\\Media" );

	// Scene setup
	ICamera* camera = myEngine->CreateCamera( kManual, 0, 0, -200 );
	IMesh* sphereMesh = myEngine->LoadMesh( "sphere.x" );

	// Use math class vectors to hold key positions / velocities
	CVector3 position( 0, 60, 0 ); // Position of orbiting object
	CVector3 velocity( 40, 0, 0 ); // Velocity of orbiting object
	CVector3 centre( 0, 0, 0 );    // Center of object being orbited

	// TODO: Create two spheres, a fixed one at 'centre' (variables above) and scaled by 5,
	// the second orbiting one at 'position' (not scaled)
	//...


	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning() && !myEngine->KeyHit( Key_Escape ))
	{
		// Draw the scene
		myEngine->DrawScene();

		// Scene update
		float updateTime = myEngine->Timer();

		// TODO: Call orbit method you wrote above to update object position, then set the orbiting model position
		//...
	}

	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}
