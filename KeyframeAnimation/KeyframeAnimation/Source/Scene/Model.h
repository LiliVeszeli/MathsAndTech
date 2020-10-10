/*******************************************
	Model.h

	Model class declaration
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "CMatrix4x4.h"
#include "Mesh.h"
#include "Input.h"

namespace gen
{
	
// Model class
class CModel
{
/*-----------------------------------------------------------------------------------------
	Constructors/Destructors
-----------------------------------------------------------------------------------------*/
public:
	// Model constructor needs a pointer to the mesh of which it is an instance
	CModel
	(
		CMesh* mesh, 
		const CVector3& pos = CVector3::kOrigin, 
		const CVector3& rot = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3& scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	~CModel();

private:
	// Disallow use of copy constructor and assignment operator (private and not defined)
	CModel( const CModel& );
	CModel& operator=( const CModel& );


/*-----------------------------------------------------------------------------------------
	Public interface
-----------------------------------------------------------------------------------------*/
public:

	/////////////////////////////////////
	// Matrix access

	// Direct access to position and matrix
	CVector3& Position( TUInt32 node = 0 )
	{
		return *(CVector3*)(&m_RelMatrices[node][3]);
	}
	CMatrix4x4& Matrix( TUInt32 node = 0 )
	{
		return m_RelMatrices[node];
	}


	/////////////////////////////////////
	// Rendering

	// Calculate the model's absolute world matrices
	void CalculateMatrices();
	
	// Render the model from the given camera
	void Render( CCamera* camera );
	

	/////////////////////////////////////
	// Control

	// Control one node in the model using keys
	void Control( TUInt32 node,
	              EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
				  EKeyCode turnCW, EKeyCode turnCCW, EKeyCode moveForward,
				  EKeyCode moveBackward, TFloat32 MoveSpeed, TFloat32 RotSpeed );


/*-----------------------------------------------------------------------------------------
	Private interface
-----------------------------------------------------------------------------------------*/
private:
	
	/*---------------------------------------------------------------------------------------------
		Data
	---------------------------------------------------------------------------------------------*/

	// Mesh of which this model is an instance
	CMesh*      m_Mesh;

	// Relative and absolute world matrices for each node - derived from the hierarchy above
	CMatrix4x4* m_RelMatrices; // Dynamically allocated arrays
	CMatrix4x4* m_Matrices;
};


} // namespace gen