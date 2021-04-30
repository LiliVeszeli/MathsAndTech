//-----------------------------------------------------
// Particle.h
//   Class encapsulating a particle, (with TL-Engine 
//   model) in a spring-based physics system
//-----------------------------------------------------

#ifndef PARTICLE_H_INCLUDED
#define PARTICLE_H_INCLUDED

#include <list>
using namespace std;

#include <TL-Engine.h>
using namespace tle;

#include "CVector3.h"
using namespace gen;

// Forward declaration - Particle and Spring classes depend on each other, must be careful with #includes
class CSpring;


class CParticle
{
public:

	/////////////////////////////
	// Constructor / Destructor

	// Constructor - creates model and shadow as well as initialising particle settings
	CParticle( IMesh* particleMesh, IMesh* shadowMesh, CVector3 position, float mass = 1.0f, bool pinned = false, unsigned int UID = DEFAULT_UID );
	~CParticle();


	////////////////////////////////////
	// Properties, getters and setters

	CVector3 GetPosition() { return m_Position; }
	float    GetMass()     { return m_Mass; }
	bool     IsPinned()    { return m_Pinned; }
	unsigned int GetUID()  { return m_UID; }
	
	void SetPosition( CVector3& position );
	void SetMass( float mass );
	void Pin( bool isPinned );

	IModel* Model()       { return m_Model; }
	IModel* Shadow()      { return m_Shadow; }


	////////////////////////////////////
	// Springs

	void AddSpring( CSpring* spring );
	void RemoveSpring( CSpring* spring );
	list<CSpring*>& GetSprings() { return m_Springs; }


	////////////////////////////////////
	// Simulation

	void InitSimulation();
	void ResetSimulation();
	void ApplyForces( float updateTime, CVector3 externalForces );


private:

	////////////////////////////////////
	// Particle data

	// Static data
	CVector3 m_Position;
	float    m_Mass;
	bool     m_Pinned;   // A pinned particle cannot be moved

	list<CSpring*> m_Springs; // All the springs attached to this particle

	IModel*  m_Model;
	IModel*  m_Shadow;

	// Simulation data
	CVector3 m_InitialPosition; // When simulation ends, particle is reset to initial position
	CVector3 m_Acceleration;
	CVector3 m_Velocity;        // For Euler or mid-point method
	CVector3 m_PrevPosition;    // For verlet method

	// UID for load/save - each particle has a UID which is saved in place of its pointer
	static const unsigned int DEFAULT_UID = 0xffffffff; // Special UID passed to constructor
	static unsigned int       m_CurrentUID;             // UID used for next new particle
	unsigned int              m_UID;                    // UID of this particle
};


#endif