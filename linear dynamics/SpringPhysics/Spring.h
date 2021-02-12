//-----------------------------------------------------
// Spring.cpp
//   Class encapsulating a spring, (with TL-Engine 
//   model) in a spring-based physics system
//-----------------------------------------------------

#ifndef SPRING_H_INCLUDED
#define SPRING_H_INCLUDED

#include <TL-Engine.h>
using namespace tle;

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
	CSpring( IMesh* springMesh, IMesh* inertialMesh, CParticle* particle1, CParticle* particle2, 
			 float coefficient, float inertialLength = 0.0f, ESpringType type = Spring );
	~CSpring();


	////////////////////////////////////
	// Properties, getters and setters

	IModel*     Model()             { return m_Model; }
	CParticle*  GetParticle1()      { return m_Particle1; }
	CParticle*  GetParticle2()      { return m_Particle2; }
	ESpringType GetType()           { return m_Type; }
	float       GetCoefficient()    { return m_SpringCoefficient; }
	float       GetInertialLength() { return m_InertialLength; }

	void SetParticle1( CParticle* particle1 );
	void SetParticle2( CParticle* particle2 );
	void SetTempTarget( CVector3& target ); // See below
	void SetType( ESpringType type );
	void SetCoefficient( float coefficient );
	void SetInertialLength( float length ); // Pass 0.0f to set to distance between particles

	void ShowInertialModel( bool showInertialModel );
	bool IsInertialModelShown() { return (m_InertialModel != 0); }


	////////////////////////////////////
	// Support functions

	// Position and scale the model to join the two particles
	void OrientateModel();


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
	IModel*     m_Model;
	ESpringType m_Type;
	CParticle*  m_Particle1;
	CParticle*  m_Particle2;

	// Model used to show inertial length (can be shown/hidden)
	IMesh*      m_InertialMesh;
	IModel*     m_InertialModel;

	CVector3    m_TempTarget; // Used to help position spring before it has been fully attached

	// Simulation data
	float       m_InertialLength;
	float       m_SpringCoefficient;

};


#endif