//-----------------------------------------------------
// Spring.cpp
//   Class encapsulating a spring, (or rod, string or
//   elastic) in a spring-based physics system
//-----------------------------------------------------

#include "Spring.h"
#include "Particle.h"


/////////////////////////////
// Static Data / Constants

// Global springiness allows simple tweak to springiness of everything in the system
const float GLOBAL_SPRINGINESS = 0.5f;

// UID for load/save - each spring has a UID which is saved in place of its pointer
unsigned int CSpring::m_CurrentUID = 0;


/////////////////////////////
// Constructor / Destructor

// Construct spring, if inertial length passed as 0, then defaults to distance between particles
// Pass UID for spring or DEFAULT_UID to create a new UID
CSpring::CSpring( CParticle* particle1, CParticle* particle2, float coefficient, float inertialLength /*= 0.0f*/,
                  ESpringType type /*= Spring*/, unsigned int UID /*= DEFAULT_UID*/ )
{
	m_Type = type;
	m_Particle1 = particle1;
	m_Particle2 = particle2;
	m_SpringCoefficient = coefficient;
	SetInertialLength( inertialLength );

	// Set UID for spring. If constructor passed DEFAULT_UID, then create a new UID
	if (UID == DEFAULT_UID)
	{
		m_UID = m_CurrentUID;
		m_CurrentUID++;
	}
	else
	{
		m_UID = UID;
		if (m_UID >= m_CurrentUID) // Ensure next new UID will be unique - beyond the largest explicitly set UID
		{
			m_CurrentUID = m_UID + 1;
		}
	}
}

CSpring::~CSpring()
{
	if (m_Particle1) m_Particle1->RemoveSpring( this );
	if (m_Particle2) m_Particle2->RemoveSpring( this );
}


////////////////////////////////////
// Properties, getters and setters

void CSpring::SetInertialLength( float length )
{
	if (length == 0.0f && m_Particle1 && m_Particle2)
	{
		m_InertialLength = Distance( m_Particle1->GetSimPosition(), m_Particle2->GetSimPosition() );
	}
	else
	{
		m_InertialLength = length;
	}
}



////////////////////////////////////
// Simulation

// TODO: Return current force exerted by spring on given particle - based on how long it currently is compared to its inertial length
// Look at the Spring.h & Particle.h files to see the available member data and methods
CVector3 CSpring::CalculateForce( CParticle* particle )
{
	// Return zero force if spring not attached to given particle, or if given particle is pinned
	if (!particle || (particle != m_Particle1 && particle != m_Particle2) || particle->IsPinned())
	{
		return CVector3::kZero;
	}

	// Calculate strength of force based on current spring length and inertial length
	float forceStrength;
	//...
	CVector3 springVec( m_Particle1->GetSimPosition(), m_Particle2->GetSimPosition() );
	float currLength = Length( springVec );
	forceStrength = (currLength - m_InertialLength) * m_SpringCoefficient * GLOBAL_SPRINGINESS;

	// Take into account type (spring, elastic, etc.)
	if ((m_Type == Elastic && forceStrength < 0) || m_Type == String || m_Type == Rod) 
	{
		forceStrength = 0;
	}

	// Return force as vector - ensure direction is correct for the given particle. Length of vector should be force strength from above
	CVector3 force = forceStrength * springVec / currLength;
	return (particle == m_Particle1) ? force : -force;
}


// Update position of the particles attached to this spring based on any constraints. The constraints used in this
// system are: rods cannot change length (always = inertial length), and strings cannot become > inertial length
void CSpring::ApplyConstraints()
{
	// No constraints on springs or elastic, so just return on those types
	if (m_Type == Spring || m_Type == Elastic)
	{
		return;
	}

	// Calculate current length of spring, and difference between that and the inertial length.
	// Return if we have a string that is shorter than the inertial length - no constraints in that case
	float springLen = Distance(m_Particle1->GetSimPosition(), m_Particle2->GetSimPosition());
	float lengthDiff = springLen - m_InertialLength;
	if (m_Type == String && lengthDiff < 0)
	{
		return;
	}

	// Correct particle positions so the length is correct again. Refer to lecture notes
	// Ensure that pinned particles are not moved
	CVector3 correction = CVector3(m_Particle1->GetSimPosition(), m_Particle2->GetSimPosition());
	correction *= lengthDiff / springLen;
	if (m_Particle1->IsPinned())
	{
		m_Particle2->SetSimPosition( m_Particle2->GetSimPosition() - correction );
	}
	else if (m_Particle2->IsPinned())
	{
		m_Particle1->SetSimPosition( m_Particle1->GetSimPosition() + correction );
	}
	else
	{
		// Simple improvement: When both particle positions require correction, make the lighter one move more. The idea 
		// is that the lighter one would be more suseptible to movement by the forces transferred across the constraint (F=ma)
		float totalMass = m_Particle1->GetMass() + m_Particle2->GetMass();
		float c1 = 1.0f - m_Particle1->GetMass() / totalMass;
		float c2 = 1.0f - m_Particle2->GetMass() / totalMass;
		m_Particle1->SetSimPosition( m_Particle1->GetSimPosition() + correction * c1 );
		m_Particle2->SetSimPosition( m_Particle2->GetSimPosition() - correction * c2 );
	}
}
 