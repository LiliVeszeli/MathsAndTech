//-----------------------------------------------------
// Particle.cpp
//   Class encapsulating a particle, (with TL-Engine 
//   model) in a spring-based physics system
//-----------------------------------------------------

#include <algorithm>
using namespace std;

#include "Particle.h"
#include "Spring.h"


/////////////////////////////
// Static Data / Constants

const float MASS_SCALE = 3.0f; // Scale given to particle of mass 1
const float DAMPING    = 1.0f; // Damping used for particle motion 

// UID for load/save - each particle has a UID which is saved in place of its pointer
unsigned int CParticle::m_CurrentUID = 0;

// Height at which to draw shadows
extern float FloorHeight;
	
/////////////////////////////
// Constructor / Destructor

// Constructor - creates model and shadow as well as initialising particle settings
CParticle::CParticle( IMesh* particleMesh, IMesh* shadowMesh, CVector3 position, float mass /*= 1.0f*/, bool pinned /*= false*/, unsigned int UID /*= DEFAULT_UID*/ )
{
	m_Model = particleMesh->CreateModel( m_Position.x, m_Position.y, m_Position.z );
	m_Shadow = shadowMesh->CreateModel( m_Position.x, FloorHeight, m_Position.z );
	SetPosition( position );
	SetMass( mass );
	Pin( pinned );

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

CParticle::~CParticle()
{
	m_Model->GetMesh()->RemoveModel( m_Model );
	m_Shadow->GetMesh()->RemoveModel( m_Shadow );

	// Detach any springs connected to this particle
	list<CSpring*>::iterator itSpring = m_Springs.begin();
	while (itSpring != m_Springs.end())
	{
		if ((*itSpring)->GetParticle1() == this) (*itSpring)->SetParticle1( NULL );
		if ((*itSpring)->GetParticle2() == this) (*itSpring)->SetParticle2( NULL );
		itSpring++;
	}
}


////////////////////////////////////
// Properties, getters and setters

void CParticle::SetPosition( CVector3& position )
{
	m_Position = position;
	m_Model->SetPosition( m_Position.x, m_Position.y, m_Position.z );
	m_Shadow->SetPosition( m_Position.x, FloorHeight, m_Position.z );
	
	// Update any attached springs
	list<CSpring*>::iterator itSpring = m_Springs.begin();
	while (itSpring != m_Springs.end())
	{
		// Redraw model
		(*itSpring)->OrientateModel();
		itSpring++;
	}
}

void CParticle::SetMass( float mass )
{
	m_Mass = mass;
	m_Model->ResetScale();
	m_Model->Scale( MASS_SCALE * Pow( m_Mass, 0.3333f ) );
	m_Shadow->ResetScale();
	m_Shadow->Scale( MASS_SCALE * Pow( m_Mass, 0.3333f ) );
}

void CParticle::Pin( bool isPinned )
{
	m_Pinned = isPinned;
	m_Model->SetSkin( m_Pinned ? "Red.jpg" : "Black.jpg" );
}


////////////////////////////////////
// Springs

void CParticle::AddSpring( CSpring* spring )
{
	m_Springs.push_back( spring );
}

void CParticle::RemoveSpring( CSpring* spring )
{
	m_Springs.remove( spring );
}


////////////////////////////////////
// Simulation

// Store initial position of particle at simulation start
void CParticle::InitSimulation()
{
	m_InitialPosition = m_Position;
	m_PrevPosition = m_Position;
	m_Velocity = CVector3::kZero;
	m_Acceleration = CVector3::kZero;
}

// Reset initial position of particle at simulation end
void CParticle::ResetSimulation()
{
	SetPosition( m_InitialPosition );
	m_PrevPosition = m_Position;
	m_Velocity = CVector3::kZero;
	m_Acceleration = CVector3::kZero;
}

// TODO: Update position & velocity of particle based on external forces plus forces from all attached springs
// Look at the Spring.h & Particle.h files to see the available member data and methods
void CParticle::ApplyForces( float updateTime, CVector3 externalForces )
{
	// Do nothing if particle is pinned
	//...
	if (m_Pinned) return;

	// Iterate through list of attached springs and get force exerted by each - accumulate into a total force
	// Also add external forces to the total
	//...
	CVector3 force = externalForces;
	list<CSpring*>::iterator itSpring = m_Springs.begin();
	while (itSpring != m_Springs.end())
	{
		CVector3 springForce = (*itSpring)->CalculateForce( this );
		force += springForce;
		itSpring++;
	}

 	// Reduce force with damping - proportional to the velocity
	//...
	force +=  -DAMPING * (m_Position - m_PrevPosition) / updateTime;

	// Get acceleration from force, then use an initial value method to update position (and velocity)
	// Call the SetPosition function with new position to update position of models
	//...
	m_Acceleration = force / m_Mass;
	
	CVector3 newPosition; 
	newPosition = 2 * m_Position - m_PrevPosition + m_Acceleration * updateTime * updateTime;
	m_PrevPosition = m_Position;
	m_Position = newPosition;

	//m_Position += updateTime * m_Velocity;
	//m_Velocity += updateTime * m_Acceleration;
	//m_Velocity *= Pow( 1 - DAMPING, updateTime );

	SetPosition( m_Position );
}