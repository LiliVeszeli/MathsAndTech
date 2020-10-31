/*******************************************
	CarEntity.h

	Car entity template and entity classes
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"

namespace gen
{

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Car Template Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A car template inherits the type, name and mesh from the base template and adds further
// car specifications
class CCarTemplate : public CEntityTemplate
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Car entity template constructor sets up the car specifications - speed, acceleration and
	// turn speed and passes the other parameters to construct the base class
	CCarTemplate
	(
		const string& type, const string& name, const string& meshFilename,
		TFloat32 maxSpeed, TFloat32 acceleration, TFloat32 turnSpeed
	) : CEntityTemplate( type, name, meshFilename )
	{
		// Set car template values
		m_MaxSpeed = maxSpeed;
		m_Acceleration = acceleration;
		m_TurnSpeed = turnSpeed;
	}

	// No destructor needed (base class one will do)


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	//	Getters

	TFloat32 GetMaxSpeed()
	{
		return m_MaxSpeed;
	}

	TFloat32 GetAcceleration()
	{
		return m_Acceleration;
	}

	TFloat32 GetTurnSpeed()
	{
		return m_TurnSpeed;
	}


/////////////////////////////////////
//	Private interface
private:

	// Common statistics for this car type (template)
	TFloat32 m_MaxSpeed;     // Maximum speed for this kind of car
	TFloat32 m_Acceleration; // Acceleration  -"-
	TFloat32 m_TurnSpeed;    // Turn speed    -"-
};



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Car Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A car entity inherits the ID/positioning/rendering support of the base entity class
// and adds instance data (just speed in this code). It overrides the update function to
// perform the car entity behaviour - very limited here
class CCarEntity : public CEntity
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Car constructor intialises car-specific data and passes its parameters to the base
	// class constructor
	CCarEntity
	(
		CCarTemplate*   carTemplate,
		TEntityUID      UID,
		const string&   name = "",
		const CVector3& position = CVector3::kOrigin, 
		const CVector3& rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3& scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	// No destructor needed


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Getters

	TFloat32 GetSpeed()
	{
		return m_Speed;
	}


	/////////////////////////////////////
	// Update

	// Update the car - performs car message processing and behaviour
	// Return false if the entity is to be destroyed
	// Keep as a virtual function in case of further derivation
	virtual bool Update( TFloat32 updateTime );
	

/////////////////////////////////////
//	Private interface
private:

	/////////////////////////////////////
	// Types

	// States available for a car - placeholders for shell code
	enum EState
	{
		Stop,
		Go,
	};


	/////////////////////////////////////
	// Data

	// The template holding common data for all car entities
	CCarTemplate* m_CarTemplate;

	// Car data
	TFloat32  m_Speed; // Current speed (in facing direction)

	// Car state
	EState m_State; // Current state
	TFloat32  m_Timer; // A timer used in the example update function   
};


} // namespace gen
