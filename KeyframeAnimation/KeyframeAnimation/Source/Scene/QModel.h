/*******************************************
	QModel.h

	Quaternion-based model class implementation
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "CMatrix4x4.h"
#include "CQuatTransform.h"
#include "Mesh.h"
#include "Input.h"

namespace gen
{
	

// Model class
class CQModel
{
/*-----------------------------------------------------------------------------------------
	Constructors/Destructors
-----------------------------------------------------------------------------------------*/
public:
	// Model constructor needs a pointer to the mesh of which it is an instance
	CQModel
	(
		CMesh* mesh,
		const CVector3& pos = CVector3::kOrigin, 
		const CVector3& rot = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3& scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	~CQModel();

private:
	// Disallow use of copy constructor and assignment operator (private and not defined)
	CQModel( const CQModel& );
	CQModel& operator=( const CQModel& );


/*-----------------------------------------------------------------------------------------
	Public interface
-----------------------------------------------------------------------------------------*/
public:

	/////////////////////////////////////
	// Transform access

	// Direct access to position and transformation
	CVector3& Position( TUInt32 node = 0 )
	{
		return m_RelTransforms[node].pos;
	}
	CQuatTransform& Transform( TUInt32 node = 0 )
	{
		return m_RelTransforms[node];
	}


	/////////////////////////////////////
	// Rendering

	// Calculate the model's absolute world transforms
	void CalculateTransforms();
	
	// Render the model from the given camera
	void Render( CCamera* camera );
	

	/////////////////////////////////////
	// Interpolation

	// Interpolate the current transforms between two given keyframes
	void KeyFrameLerp
	(
		TUInt32  keyFrame1,
		TUInt32  keyFrame2,
		TFloat32 blend      // Value from 0.0f to 1.0f
	);

	// Spherical linearly interpolate the current transforms between two given keyframes
	void KeyFrameSlerp
	(
		TUInt32  keyFrame1,
		TUInt32  keyFrame2,
		TFloat32 blend      // Value from 0.0f to 1.0f
	);


	// Read transforms from a text file into a given keyframe
	void ReadKeyFrame
	(
		TUInt32       keyFrame,
		const string& fileName
	);

	// Output current transforms to a text file
	void WriteTransforms
	(
		const string& fileName
	);


/*-----------------------------------------------------------------------------------------
	Private interface
-----------------------------------------------------------------------------------------*/
private:
	
	/*---------------------------------------------------------------------------------------------
		Data
	---------------------------------------------------------------------------------------------*/

	// Mesh of which this model is an instance
	CMesh*          m_Mesh;


	// Define a set of keyframes for a single node
	static const TUInt32   MaxKeyFrames = 10;
	typedef CQuatTransform TKeyFrameSet[MaxKeyFrames];

	// Sets of keyframes to use for transforms
	TKeyFrameSet*   m_KeyFrameSets;


	// Relative and absolute world transforms for each node - quaternion-based, not matrices
	CQuatTransform* m_RelTransforms; // Dynamically allocated arrays
	CQuatTransform* m_Transforms;

	// Actual matrices used for rendering - calculated from absolute transform array
	CMatrix4x4*     m_Matrices;
};


} // namespace gen