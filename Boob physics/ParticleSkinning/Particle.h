//-----------------------------------------------------
// Particle.h
//   Class encapsulating a particle in a spring-based
//   physics system
//-----------------------------------------------------

#ifndef PARTICLE_H_INCLUDED
#define PARTICLE_H_INCLUDED

#include <list>
using namespace std;

#include "CVector3.h"
#include "CMatrix4x4.h"
using namespace gen;

// Forward declaration - Particle and Spring classes depend on each other, must be careful with #includes
class CSpring;


class CParticle
{
public:

	/////////////////////////////
	// Constructor / Destructor

	// Constructor - creates model and shadow as well as initialising particle settings
	CParticle( CVector3 position, float mass = 1.0f, bool pinned = false, unsigned int UID = DEFAULT_UID );
	~CParticle();


	////////////////////////////////////
	// Properties, getters and setters

	CVector3   GetModelPosition() { return m_ModelPosition; }
	CVector3   GetSimPosition()   { return m_SimPosition; }
	CMatrix4x4 GetMatrix( CMatrix4x4 modelMatrix, bool getSimulationMatrix ); // Get a full matrix for the particle, deriving axes from attached springs (see cpp file)

	float    GetMass()    { return m_Mass; }
	bool     IsPinned()   { return m_Pinned; }
	unsigned int GetUID() { return m_UID; }
	
	void SetSimPosition( CVector3& position )   { m_SimPosition = position; }
	
	void SetMass( float mass ) { m_Mass = mass; }
	void Pin( bool isPinned )  { m_Pinned = isPinned; } 


	////////////////////////////////////
	// Springs

	void AddSpring( CSpring* spring );
	void RemoveSpring( CSpring* spring );
	list<CSpring*>& GetSprings() { return m_Springs; }


	////////////////////////////////////
	// Model Interaction

	void Initialise( CMatrix4x4 matrix );
	void Transform( CMatrix4x4 matrix );


	////////////////////////////////////
	// Simulation

	void ApplyForces( float updateTime, CVector3 externalForces );


private:

	////////////////////////////////////
	// Particle data

	//****| INFO |**************************************************************************************************************//
	// Particle position storage is more complex as system follows a model - see lecture notes and comment in Particle.cpp
	//**************************************************************************************************************************//
	CVector3 m_ModelPosition; // Original particle position as designed (model space)
	CVector3 m_WorldPosition; // Original particle position transformed to world space (to follow model), but with no simulation
	CVector3 m_SimPosition;   // Particle position in world space with simulation applied

	// Other Static data
	float    m_Mass;
	bool     m_Pinned; // A pinned particle cannot be moved
	
	list<CSpring*> m_Springs; // All the springs attached to this particle

	// Simulation data
	CVector3 m_Acceleration;
	CVector3 m_PrevPosition;

	// UID for load/save - each particle has a UID which is saved in place of its pointer
	static const unsigned int DEFAULT_UID = 0xffffffff; // Special UID passed to constructor
	static unsigned int       m_CurrentUID;             // UID used for next new particle
	unsigned int              m_UID;                    // UID of this particle
};


#endif