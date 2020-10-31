/*******************************************
	CarEntity.cpp

	Car entity template and entity classes
********************************************/

#include "CarEntity.h"
#include "EntityManager.h"

namespace gen
{

// Reference to entity manager from Portals.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix()[3];
extern CEntityManager EntityManager;

// Will need a similar extern for the messenger class object, when it is has been added

// Helper function made available from Portals.cpp - returns a random car UID. Needed to
// implement the car behaviour in the Update function below
extern TEntityUID RandomCar();



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Car Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Car constructor intialises car-specific data and passes its parameters to the base
// class constructor
CCarEntity::CCarEntity
(
	CCarTemplate*   carTemplate,
	TEntityUID      UID,
	const string&   name /*=""*/,
	const CVector3& position /*= CVector3::kOrigin*/, 
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( carTemplate, UID, name, position, rotation, scale )
{
	m_CarTemplate = carTemplate;

	// Initialise car data and state
	m_Speed = 0.0f;
	m_State = Go;
	m_Timer = 0.0f;
}


// Update the car - controls its behaviour. The shell code just performs some test behaviour
// Return false if the entity is to be destroyed
bool CCarEntity::Update( TFloat32 updateTime )
{
	// Only move if in Go state (although this shell code doesn't ever change state)
	if (m_State == Go)
	{
		// Cycle speed up and down using a sine wave - just test behaviour
		m_Speed = 10.0f * Sin( m_Timer * 4.0f );
		m_Timer += updateTime;
	}
	else
	{
		m_Speed = 0;
	}

	// Perform movement...
	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ( m_Speed * updateTime );
	
	// Calculate speed to rotate wheels based on speed
	const TFloat32 wheelDiameter = 0.8f;
	TFloat32 wheelSpeed = ToRadians(360.0f) * m_Speed * updateTime / (wheelDiameter * kfPi);

	// Rotate each wheel - meshes have been arranged so nodes 3->6 are the wheels
	Matrix(3).RotateLocalX( wheelSpeed );
	Matrix(4).RotateLocalX( wheelSpeed );
	Matrix(5).RotateLocalX( wheelSpeed );
	Matrix(6).RotateLocalX( wheelSpeed );

	return true;
}


} // namespace gen
