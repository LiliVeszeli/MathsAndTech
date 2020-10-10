/*******************************************
	QModel.cpp

	Quaternion-based model class implementation
********************************************/

#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "QModel.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

// Model constructor needs a pointer to the mesh of which it is an instance
CQModel::CQModel
(
	CMesh* mesh,
	const CVector3& pos /*= CVector3::kOrigin*/, 
	const CVector3& rot /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
)
{
	m_Mesh = mesh;

	// Allocate space for transforms and matrices
	TUInt32 numNodes = m_Mesh->GetNumNodes();
	m_RelTransforms = new CQuatTransform[numNodes];
	m_Transforms = new CQuatTransform[numNodes];
	m_Matrices = new CMatrix4x4[numNodes];

	// Set initial transforms from mesh defaults
	for (TUInt32 node = 0; node < numNodes; ++node)
	{
		m_RelTransforms[node] = CQuatTransform( m_Mesh->GetNode( node ).positionMatrix );
	}

	// Override root transform with constructor parameters
	CMatrix4x4 transformMatrix( pos, rot, kZXY, scale );
	m_RelTransforms[0] = CQuatTransform( transformMatrix );

	// Allocate space for and initialise keyframes
	m_KeyFrameSets = new TKeyFrameSet[numNodes];
}


CQModel::~CQModel()
{
	delete[] m_KeyFrameSets;
	delete[] m_Matrices;
	delete[] m_Transforms;
	delete[] m_RelTransforms;
}


//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------

// Calculate the model's absolute world matrices
void CQModel::CalculateTransforms()
{
	// Calculate absolute transforms from relative transforms & node heirarchy
	m_Transforms[0] = m_RelTransforms[0];
	for (TUInt32 node = 1; node < m_Mesh->GetNumNodes(); ++node)
	{
		m_Transforms[node] = m_RelTransforms[node] * m_Transforms[m_Mesh->GetNode( node ).parent];
	}

	// Incorporate any bone<->mesh offsets (only relevant for skinning)
	// Don't need this step for this exercise
}


// Render the model from the given camera
void CQModel::Render( CCamera* camera )
{
	// Calculate the model's current absolute transforms
	CalculateTransforms();

	// Convert the quaternion based transforms to matrices just before rendering
	for (TUInt32 node = 0; node < m_Mesh->GetNumNodes(); ++node)
	{
		m_Transforms[node].GetMatrix( m_Matrices[node] );
	}
	m_Mesh->Render( m_Matrices, camera );
}


//-----------------------------------------------------------------------------
// Interpolation 
//-----------------------------------------------------------------------------

// Linearly interpolate the current transforms between two given keyframes
void CQModel::KeyFrameLerp
(
	TUInt32  keyFrame1,
	TUInt32  keyFrame2,
	TFloat32 blend      // Value from 0.0f to 1.0f
)
{
	// For each node in the model hierarchy...
	for (TUInt32 node = 1; node < m_Mesh->GetNumNodes(); ++node)
	{
		// Create the relative transform by interpolating the two keyframes
		Lerp( m_KeyFrameSets[node][keyFrame1], m_KeyFrameSets[node][keyFrame2], blend,
		      m_RelTransforms[node] );
	}
}


// Spherical linearly interpolate the current transforms between two given keyframes
void CQModel::KeyFrameSlerp
(
	TUInt32  keyFrame1,
	TUInt32  keyFrame2,
	TFloat32 blend      // Value from 0.0f to 1.0f
)
{
	// For each node in the model hierarchy...
	for (TUInt32 node = 1; node < m_Mesh->GetNumNodes(); ++node)
	{
		// Create the relative transform by interpolating the two keyframes
		Slerp( m_KeyFrameSets[node][keyFrame1], m_KeyFrameSets[node][keyFrame2], blend,
		       m_RelTransforms[node] );
	}
}


// Read transforms from a text file into a given keyframe
void CQModel::ReadKeyFrame
(
	TUInt32       keyFrame,
	const string& fileName
)
{
	// Open file stream for reading
	fstream fileIn( fileName.c_str(), ios::in );

	// Read and check number of nodes
	TUInt32 nodes;
	fileIn >> nodes;
	if (nodes != m_Mesh->GetNumNodes())
	{
		return;
	}

	// Read each node
	char tmpChar;
	TUInt32 tmpInt;
	for (TUInt32 node = 0; node < m_Mesh->GetNumNodes(); ++node)
	{
		const CQuatTransform& t = m_KeyFrameSets[node][keyFrame];
		fileIn >> tmpInt;
		fileIn >> tmpChar >> (float)t.pos.x >> tmpChar >> (float)t.pos.y >> tmpChar
		                  >> (float)t.pos.z >> tmpChar;
		fileIn >> tmpChar >> (float)t.quat.w >> tmpChar >> (float)t.quat.x >> tmpChar
		                  >> (float)t.quat.y >> tmpChar >> (float)t.quat.z >> tmpChar;
		fileIn >> tmpChar >> (float)t.scale.x >> tmpChar >> (float)t.scale.y >> tmpChar
		                  >> (float)t.scale.z >> tmpChar;
	}

	// Close file stream
	fileIn.close();
}


// Output current transforms to a text file
void CQModel::WriteTransforms
(
	const string& fileName
)
{
	// Open file stream for writing
	fstream fileOut( fileName.c_str(), ios::out );

	// Output number of nodes
	fileOut << m_Mesh->GetNumNodes() << endl << endl;

	// Output each node
	for (TUInt32 node = 0; node < m_Mesh->GetNumNodes(); ++node)
	{
		const CQuatTransform& t = m_RelTransforms[node];
		fileOut << node << endl;
		fileOut << "  (" << t.pos.x << ", " << t.pos.y << ", " << t.pos.z << ")" << endl;
		fileOut << "  (" << t.quat.w << ", " << t.quat.x << ", " << t.quat.y << ", " 
		                 << t.quat.z << ")" << endl;
		fileOut << "  (" << t.scale.x << ", " << t.scale.y << ", " << t.scale.z << ")" << endl;
		fileOut << endl;
	}

	// Close file stream
	fileOut.close();
}


} // namespace gen
