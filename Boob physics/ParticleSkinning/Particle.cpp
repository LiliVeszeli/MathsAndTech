//-----------------------------------------------------
// Particle.cpp
//   Class encapsulating a particle in a spring-based
//   physics system
//-----------------------------------------------------

//#include <algorithm>
//using namespace std;

#include "Particle.h"
#include "Spring.h"


/////////////////////////////
// Static Data / Constants

// Damping used for particle motion 
// Tweak depending on system
const float DAMPING = 0.5f;

// UID for load/save - each particle has a UID which is saved in place of its pointer
unsigned int CParticle::m_CurrentUID = 0;

	
/////////////////////////////
// Constructor / Destructor

// Constructor
CParticle::CParticle( CVector3 position, float mass /*= 1.0f*/, bool pinned /*= false*/, unsigned int UID /*= DEFAULT_UID*/ )
{
	m_ModelPosition = m_WorldPosition = m_SimPosition = position;
	m_Mass = mass;
	m_Pinned = pinned;

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
// Model Interaction

//****| INFO |*************************************************************************************//
// The particle physics system needs to move around with the model it is attached to. So particles
// have functions to do that, and their positions are managed differently:
// - The original particle positions are effectively in model space, as they were designed in
//   reference to the model
// - At setup time we use the initial world matrix of the model to transform all these particle
//   positions into world space to match model position
// - Each frame we use the updated world matrix of the model to update the particle world space
//   positions. However, only the pinned particles are actually moved directly to these new world 
//   space positions. The remaining particles will follow as appropriate in the simulation step
//   due to spring forces, constraints etc.
// - Particle are simulated by position, but can return a matrix to the rendering code. The axes
//   of this matrix are derived from the attached springs. Can return the matrix for the original
//   modelled position, or the current simulation position. Together these matrices can give us a
//   true world matrix for each particle, allowing particles to work as bones in the skinning code
//*************************************************************************************************//

// Initialise all particle data at simulation start. Pass initial world matrix of model that system is attached to
void CParticle::Initialise( CMatrix4x4 worldMatrix )
{
	// Use given matrix to set position of particle in world space to match the model's position
	m_WorldPosition = worldMatrix.TransformPoint( m_ModelPosition );
	m_SimPosition = m_WorldPosition;
	m_PrevPosition = m_SimPosition;
	m_Acceleration = CVector3::kZero;
}

// Calculate new positions for particles given world matrix of model that system is attached to
void CParticle::Transform( CMatrix4x4 worldMatrix )
{
	m_WorldPosition = worldMatrix.TransformPoint( m_ModelPosition );
	
	// Only move pinned particles directly to this new world space position (see comment above)
	if (m_Pinned)
	{
		m_SimPosition = m_WorldPosition;
	}
}

CVector3 ChooseNonAlignedVector( CVector3 src, CVector3 v1, CVector3 v2 )
{
	return (Abs( Dot( Normalise(src), Normalise(v2) )) > 0.99f) ? v1 : v2;
}

// Get a matrix defining position and rotation for this particle. As particles are defined by position only, the rotational part of the matrix
// is derived by looking at the directions of the attached springs. The particle will attempt to face in the direction of it's first spring,
// with it's other local axes derived from its second spring (uses facing matrix approach). Uses the model axes where there are no springs.
// Ideally, we would be able to specify which spring(s) define the facing of the matrix to give the best rotational behaviour for the model. 
//
// Having a full matrix per-particle will allow use the standard skinning algorithm when rendering - each particle representing a bone.
// In practice there are two versions of this matrix - the initial matrix (as modelled), and the current matrix in the simulation
CMatrix4x4 CParticle::GetMatrix( CMatrix4x4 modelMatrix, bool getSimulationMatrix )
{
	// No springs, return world axis aligned matrix with correct position
	if (m_Springs.size() == 0) 
	{
		return CMatrix4x4( (getSimulationMatrix ? m_SimPosition : m_ModelPosition) );
	}

	// At least one spring, matrix Z-axis faces down spring, use cross products with world axes for remainder of matrix
	list<CSpring*>::iterator itSpring = m_Springs.begin();

	// Get direction of spring from this particle, either as originally modelled or at current time in simulation (need to check which end this particle on)
	CVector3 springDir = getSimulationMatrix ? ((*itSpring)->GetParticle1()->GetSimPosition() - (*itSpring)->GetParticle2()->GetSimPosition()) :
		                                       ((*itSpring)->GetParticle1()->GetModelPosition() - (*itSpring)->GetParticle2()->GetModelPosition());
	if ((*itSpring)->GetParticle1() == this) springDir = -springDir;

	// Use facing matrix helper from matrix class. Spring direction is first axis for matrix, use model X-axis to determine second axis (with cross products)
	return MatrixFaceDirection( (getSimulationMatrix ? m_SimPosition : m_ModelPosition), springDir, modelMatrix.XAxis() );
}


////////////////////////////////////
// Simulation

// Update position & velocity of particle based on external forces plus forces from all attached springs
// Look at the Spring.h & Particle.h files to see the available member data and methods
void CParticle::ApplyForces( float frameTime, CVector3 externalForces )
{
	// Do nothing if particle is pinned
	if (m_Pinned) return;

	// Iterate through list of attached springs and get force exerted by each - accumulate into a total force
	// Also add external forces to the total
	CVector3 force = externalForces;
	list<CSpring*>::iterator itSpring = m_Springs.begin();
	while (itSpring != m_Springs.end())
	{
		CVector3 springForce = (*itSpring)->CalculateForce( this );
		force += springForce;
		itSpring++;
	}

 	// Reduce force with damping - proportional to the velocity
	force -=  DAMPING * (m_SimPosition - m_PrevPosition) / frameTime;

	// Get acceleration from force
	m_Acceleration = force / m_Mass;
	
	// Update position using verlet method
	CVector3 newPosition = 2 * m_SimPosition - m_PrevPosition + m_Acceleration * frameTime * frameTime;
	m_PrevPosition = m_SimPosition;
	m_SimPosition = newPosition;
}