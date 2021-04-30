//-----------------------------------------------------
// Spring.cpp
//   Class encapsulating a spring, (or rod, string or
//   elastic) in a spring-based physics system
//-----------------------------------------------------

#ifndef SPRING_H_INCLUDED
#define SPRING_H_INCLUDED

#include "CVector3.h"
using namespace gen;

// Forward declaration - Particle and Spring classes depend on each other, must be careful with #includes
class CParticle;


class CSpring
{
public:

	// Springs are actually of several forms
	enum ESpringType
	{
		Spring = 0, // Force on squash or stretch
		Elastic,    // No resistance to squash, force on stretch
		String,     // No resistance to squash, cannot be stretched
		Rod,        // Cannot be squashed or stretched
		NumTypes
	};

	/////////////////////////////
	// Constructor / Destructor

	// Construct spring, if inertial length passed as 0, then defaults to distance between particles
	CSpring( CParticle* particle1, CParticle* particle2, float coefficient, float inertialLength = 0.0f,
	         ESpringType type = Spring, unsigned int UID = DEFAULT_UID );
	~CSpring();


	////////////////////////////////////
	// Properties, getters and setters

	CParticle*   GetParticle1()      { return m_Particle1; }
	CParticle*   GetParticle2()      { return m_Particle2; }
	ESpringType  GetType()           { return m_Type; }
	float        GetCoefficient()    { return m_SpringCoefficient; }
	float        GetInertialLength() { return m_InertialLength; }
	unsigned int GetUID()            { return m_UID; }

	void SetParticle1( CParticle* particle1 ) { m_Particle1 = particle1; }
	void SetParticle2( CParticle* particle2 ) { m_Particle2 = particle2; }
	void SetType( ESpringType type )          { m_Type = type; };
	void SetCoefficient( float coefficient )  { m_SpringCoefficient = coefficient; }
	void SetInertialLength( float length ); // Pass 0.0f to set to distance between particles


	////////////////////////////////////
	// Simulation

	// Return current force exerted by spring on given particle - based on how long it currently is compared to its inertial length
	CVector3 CalculateForce( CParticle* particle );

	// Update position of the particles attached to this spring based on any constraints. The constraints used in this
	// system are: rods cannot change length (always = inertial length), and strings cannot become > inertial length
	void ApplyConstraints();


private:

	////////////////////////////////////
	// Spring data

	// Static data
	ESpringType m_Type;
	CParticle*  m_Particle1;
	CParticle*  m_Particle2;

	// Simulation data
	float       m_InertialLength;
	float       m_SpringCoefficient;

	// UID for load/save - each spring has a UID which is saved in place of its pointer
	static const unsigned int DEFAULT_UID = 0xffffffff; // Special UID passed to constructor
	static unsigned int       m_CurrentUID;             // UID used for next new spring
	unsigned int              m_UID;                    // UID of this spring
};


#endif