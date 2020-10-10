/*******************************************
	Model.cpp

	Matrix-based model class implementation
********************************************/

#include "Model.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

// Model constructor needs a pointer to the mesh of which it is an instance
CModel::CModel
(
	CMesh* mesh,
	const CVector3& pos /*= CVector3::kOrigin*/, 
	const CVector3& rot /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
)
{
	m_Mesh = mesh;

	// Allocate space for matrices
	TUInt32 numNodes = m_Mesh->GetNumNodes();
	m_RelMatrices = new CMatrix4x4[numNodes];
	m_Matrices = new CMatrix4x4[numNodes];

	// Set initial matrices from mesh defaults
	for (TUInt32 node = 0; node < numNodes; ++node)
	{
		m_RelMatrices[node] = m_Mesh->GetNode( node ).positionMatrix;
	}

	// Override root matrix with constructor parameters
	m_RelMatrices[0] = CMatrix4x4( pos, rot, kZXY, scale );
}


CModel::~CModel()
{
	delete[] m_Matrices;
	delete[] m_RelMatrices;
}


//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------

// Calculate the model's absolute world matrices
void CModel::CalculateMatrices()
{
	// Calculate absolute matrices from relative node matrices & node heirarchy
	m_Matrices[0] = m_RelMatrices[0];
	for (TUInt32 node = 1; node < m_Mesh->GetNumNodes(); ++node)
	{
		m_Matrices[node] = m_RelMatrices[node] * m_Matrices[m_Mesh->GetNode( node ).parent];
	}

	// Incorporate any bone<->mesh offsets (only relevant for skinning)
	// Don't need this step for this exercise
}


// Render the model from the given camera
void CModel::Render( CCamera* camera )
{
	// Calculate the model's current absolute matrices
	CalculateMatrices();

	// Render with absolute matrices
	m_Mesh->Render( m_Matrices, camera );
}


//-----------------------------------------------------------------------------
// Control
//-----------------------------------------------------------------------------

// Control a single model node using keys
void CModel::Control( TUInt32 node,
                      EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight, 
					  EKeyCode turnCW, EKeyCode turnCCW, EKeyCode moveForward, 
					  EKeyCode moveBackward, TFloat32 MoveSpeed, TFloat32 RotSpeed )
{
	if (KeyHeld( turnDown ))
	{
		m_RelMatrices[node].RotateWorldX( RotSpeed );
	}
	if (KeyHeld( turnUp ))
	{
		m_RelMatrices[node].RotateWorldX( -RotSpeed );
	}
	if (KeyHeld( turnRight ))
	{
		m_RelMatrices[node].RotateWorldY( RotSpeed );
	}
	if (KeyHeld( turnLeft ))
	{
		m_RelMatrices[node].RotateWorldY( -RotSpeed );
	}
	if (KeyHeld( turnCW ))
	{
		m_RelMatrices[node].RotateWorldZ( RotSpeed );
	}
	if (KeyHeld( turnCCW ))
	{
		m_RelMatrices[node].RotateWorldZ( -RotSpeed );
	}

	// Local Z movement - move in the direction of the Z axis
	if (KeyHeld( moveForward ))
	{
		m_RelMatrices[node].MoveLocalZ( MoveSpeed );
	}
	if (KeyHeld( moveBackward ))
	{
		m_RelMatrices[node].MoveLocalZ( -MoveSpeed );
	}
}


} // namespace gen