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
// Constructor / Destructor

// Constructor - creates model and shadow as well as initialising particle settings
CParticle::CParticle( IMesh* particleMesh, IMesh* shadowMesh, CVector3 position, float mass /*= 1.0f*/, bool pinned /*= false*/ )
{
	m_Position = position;
	m_Mass = mass;
	m_Pinned = pinned;

	m_Model = particleMesh->CreateModel( m_Position.x, m_Position.y, m_Position.z );
	m_Model->ResetScale();
	m_Model->Scale( m_Mass );

	m_Shadow = shadowMesh->CreateModel( m_Position.x, 0, m_Position.z );
	m_Shadow->ResetScale();
	m_Shadow->Scale( m_Mass );
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
	m_Shadow->SetPosition( m_Position.x, 0, m_Position.z );
	
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
	m_Model->Scale( 10.0f * Pow( m_Mass, 0.3333f ) );
	m_Shadow->ResetScale();
	m_Shadow->Scale( 10.0f * Pow( m_Mass, 0.3333f ) );
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

// Store initial position of particle at simuilation start
void CParticle::InitSimulation()
{
	m_InitialPosition = m_Position;
	m_PrevPosition = m_Position;
	m_Velocity = CVector3::kZero;
	m_Acceleration = CVector3::kZero;
}

// Reset initial position of particle at simuilation end
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


	// Iterate through list of attached springs and get force exerted by each - accumulate into a total force
	// Also add external forces to the total
	//...


	// Get acceleration from force, then use an initial value method to update position (and velocity)
	// Call the SetPosition function with new position to update position of models
	//...
}