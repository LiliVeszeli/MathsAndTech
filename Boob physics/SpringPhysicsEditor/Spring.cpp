//-----------------------------------------------------
// Spring.cpp
//   Class encapsulating a spring, (with TL-Engine 
//   model) in a spring-based physics system
//-----------------------------------------------------

#include "Spring.h"
#include "Particle.h"

#include "CMatrix4x4.h"
using namespace gen;


/////////////////////////////
// Static Data / Constants

const float FLOPPY_LENGTH = 0.99f; // Ratio of original length below which elastic or string becomes "floppy"
const float SPRING_WIDTH  = 7.0f; // Width of spring models
const float RULER_WIDTH   = 0.05f;  // Width of inertial length ruler given coefficient 1

// UID for load/save - each spring has a UID which is saved in place of its pointer
unsigned int CSpring::m_CurrentUID = 0;


/////////////////////////////
// Constructor / Destructor

// Construct spring, if inertial length passed as 0, then defaults to distance between particles
// Pass UID for spring or DEFAULT_UID to create a new UID
CSpring::CSpring( IMesh* springMesh, IMesh* inertialMesh, CParticle* particle1, CParticle* particle2, 
                  float coefficient, float inertialLength /*= 0.0f*/, ESpringType type /*= Spring*/, unsigned int UID /*= DEFAULT_UID*/ )
{
	m_Particle1 = particle1;
	m_Particle2 = particle2;
	m_TempTarget = CVector3::kOrigin;

	m_Model = springMesh->CreateModel();
	SetType( type );
	m_InertialMesh = inertialMesh; // Inertial model used to show inertial length, not shown at first
	m_InertialModel = 0;

	m_SpringCoefficient = coefficient;
	if (inertialLength > 0.0f)
	{
		m_InertialLength = inertialLength;
	}
	else if (m_Particle1 && m_Particle2)
	{
		m_InertialLength = Distance( m_Particle1->GetPosition(), m_Particle2->GetPosition() );
	}
	else
	{
		m_InertialLength = 0.0f;
	}

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

	OrientateModel();
}

CSpring::~CSpring()
{
	m_Model->GetMesh()->RemoveModel( m_Model );
	if (m_Particle1) m_Particle1->RemoveSpring( this );
	if (m_Particle2) m_Particle2->RemoveSpring( this );
}


////////////////////////////////////
// Properties, getters and setters

void CSpring::SetParticle1( CParticle* particle1 )
{
	m_Particle1 = particle1;
	OrientateModel();
}

void CSpring::SetParticle2( CParticle* particle2 )
{
	m_Particle2 = particle2;
	OrientateModel();
}

void CSpring::SetTempTarget( CVector3& target )
{
	m_TempTarget = target;
	OrientateModel();
}

void CSpring::SetType( CSpring::ESpringType type )
{
	m_Type = type;
	switch (type)
	{
		case Spring: 
			m_Model->SetSkin( "spring_tlxcutout.tga" ); 
			break;
		case Elastic: 
			m_Model->SetSkin( "elastic_tlxcutout.tga" ); 
			break;
		case String: 
			m_Model->SetSkin( "string_tlxcutout.tga" ); 
			break;
		case Rod: 
			m_Model->SetSkin( "rod_tlxcutout.tga" ); 
			break;
	}
}

void CSpring::SetCoefficient( float coefficient )
{
	m_SpringCoefficient = coefficient;
	OrientateModel();
}

void CSpring::SetInertialLength( float length )
{
	if (length == 0.0f && m_Particle1 && m_Particle2)
	{
		m_InertialLength = Distance( m_Particle1->GetPosition(), m_Particle2->GetPosition() );
	}
	else
	{
		m_InertialLength = length;
	}
	OrientateModel();
}

void CSpring::ShowInertialModel( bool showInertialModel )
{
	if (showInertialModel)
	{
		if (m_InertialModel == 0) m_InertialModel = m_InertialMesh->CreateModel();
	}
	else
	{
		if (m_InertialModel != 0) m_InertialMesh->RemoveModel( m_InertialModel );
		m_InertialModel = 0;
	}
	OrientateModel();
}


////////////////////////////////////
// Support functions

// Position and scale the model to join the two particles
void CSpring::OrientateModel()
{
	if (m_Particle1)
	{
		if (m_Particle2)
		{
			// Spring is fully attached
			CMatrix4x4 springMat = MatrixFaceTarget( m_Particle1->GetPosition(), m_Particle2->GetPosition(), CVector3::kZAxis );
			springMat.ScaleZ( Distance( m_Particle1->GetPosition(), m_Particle2->GetPosition() ) );
			springMat.ScaleX( SPRING_WIDTH );
			m_Model->SetMatrix( &springMat.e00 );

			// Draw inertial length model (at mid-point of spring) - width represents spring coefficient
			if (m_InertialModel != 0)
			{
				springMat = MatrixFaceTarget( (m_Particle1->GetPosition() + m_Particle2->GetPosition()) * 0.5f,
											   m_Particle2->GetPosition(), CVector3::kZAxis );
				springMat.ScaleZ( m_InertialLength );
				springMat.ScaleX( m_SpringCoefficient * Distance( m_Particle1->GetPosition(), m_Particle2->GetPosition() ) * RULER_WIDTH );
				m_InertialModel->SetMatrix( &springMat.e00 );
			}			
			
			// Make string and elastic look "floppy" when short
			float length = Distance( m_Particle1->GetPosition(), m_Particle2->GetPosition() );
			if (length < m_InertialLength * FLOPPY_LENGTH) // "Floppy" at < certain proportion of normal length
			{
				if (m_Type == String) m_Model->SetSkin( "stringfloppy_tlxcutout.tga" ); 
				if (m_Type == Elastic) m_Model->SetSkin( "elasticfloppy_tlxcutout.tga" ); 
			}
			else
			{
				if (m_Type == String) m_Model->SetSkin( "string_tlxcutout.tga" ); 
				if (m_Type == Elastic) m_Model->SetSkin( "elastic_tlxcutout.tga" ); 
			}
			return;
		}

		// Spring is only attached at one end, attach other to temporary target
		CMatrix4x4 springMat = MatrixFaceTarget( m_Particle1->GetPosition(), m_TempTarget, CVector3::kZAxis );
		springMat.ScaleZ( Distance( m_Particle1->GetPosition(), m_TempTarget ) );
		springMat.ScaleX( SPRING_WIDTH );
		m_Model->SetMatrix( &springMat.e00 );
		if (m_InertialModel != 0)
		{
			m_InertialModel->Scale(0); // Can't show inertial length - not attached yet
		}
		return;
	}

	// Spring is not attached at all, place it at temporary target and give it default scale
	CMatrix4x4 springMat = MatrixFaceDirection( m_TempTarget, CVector3::kXAxis, CVector3::kZAxis );
	springMat.Scale( SPRING_WIDTH );
	m_Model->SetMatrix( &springMat.e00 );
	if (m_InertialModel != 0)
	{
		m_InertialModel->Scale(0); // Can't show inertial length - not attached yet
	}
}


////////////////////////////////////
// Simulation

// TODO: Return current force exerted by spring on given particle - based on how long it currently is compared to its inertial length
// Look at the Spring.h & Particle.h files to see the available member data and methods
CVector3 CSpring::CalculateForce( CParticle* particle )
{
	// Return zero force if spring not attached to given particle, or if given particle is pinned
	//...
	if (!particle || (particle != m_Particle1 && particle != m_Particle2) || particle->IsPinned())
	{
		return CVector3::kZero;
	}

	// Calculate strength of force based on current spring length and inertial length
	float forceStrength;
	//...
	CVector3 springVec( m_Particle1->GetPosition(), m_Particle2->GetPosition() );
	float currLength = Length( springVec );
	forceStrength = (currLength - m_InertialLength) * m_SpringCoefficient;

	// Take into account type (spring, elastic, etc.)
	if ((m_Type == Elastic && forceStrength < 0) || m_Type == String || m_Type == Rod) 
	{
		forceStrength = 0;
	}

	// Return force as vector - ensure direction is correct for the given particle. Length of vector should be force strength from above
	//...
	CVector3 force = forceStrength * springVec / currLength;
	if (particle == m_Particle1)
	{
		return force;
	}
	else
	{
		return -force;
	}

	//return CVector3::kZero; // temporary line to make code compile, change it when you implement this function
}


// TODO: Update position of the particles attached to this spring based on any constraints. The constraints used in this
// system are: rods cannot change length (always = inertial length), and strings cannot become > inertial length
void CSpring::ApplyConstraints()
{
	// No constraints on springs or elastic, so just return on those types
	//...
	if (m_Type == Spring || m_Type == Elastic)
	{
		return;
	}

	// Calculate current length of spring, and difference between that and the inertial length.
	// Return if we have a string that is shorter than the inertial length - no constraints in that case
	//...
	float springLen = Distance(m_Particle1->GetPosition(), m_Particle2->GetPosition());
	float lengthDiff = springLen - m_InertialLength;
	if (m_Type == String && lengthDiff < 0)
	{
		return;
	}

	// Correct particle positions so the length is correct again. Refer to lecture notes
	// Ensure that pinned particles are not moved
	//...
	CVector3 correction = CVector3(m_Particle1->GetPosition(), m_Particle2->GetPosition());
	correction *= lengthDiff / springLen;
	if (m_Particle1->IsPinned())
	{
		m_Particle2->SetPosition( m_Particle2->GetPosition() - correction );
	}
	else if (m_Particle2->IsPinned())
	{
		m_Particle1->SetPosition( m_Particle1->GetPosition() + correction );
	}
	else
	{
		// Simple improvement: When both particle positions require correction, make the lighter one move more. The idea 
		// is that the lighter one would be more suseptible to movement by the forces transferred across the constraint (F=ma)
		float totalMass = m_Particle1->GetMass() + m_Particle2->GetMass();
		float c1 = 1.0f - m_Particle1->GetMass() / totalMass;
		float c2 = 1.0f - m_Particle2->GetMass() / totalMass;
		m_Particle1->SetPosition( m_Particle1->GetPosition() + correction * c1 );
		m_Particle2->SetPosition( m_Particle2->GetPosition() - correction * c2 );
	}
	
}
 