/*******************************************
	
	QModel.h

	Quaternion-based model class declaration

********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "CMatrix4x4.h"
#include "CQuatTransform.h"
#include "Mesh.h"
#include "Animation.h"
#include "Input.h"

namespace gen
{

const TUInt32 NumAnimationSlots = 3;

// QModel class
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
	// Animation Support

	// Play a new animation at the given slot in the animation list for a model.
	// Pass an animation of 0 to remove the animation in that slot.
	// Optionally pass whether to loop the animation, its weight (compared to other animations playing
	// on the same model), initial position (0->1, within the animation) and speed multiplier
	void PlayAnimation( CAnimation* anim, TUInt32 slot, bool looping = false,
	                    TFloat32 weight = 1.0f, TFloat32 pos = 0.0f, TFloat32 speed = 1.0f );

	// Update all current animations by the given amount of time
	void UpdateAnimations( TFloat32 frameTime );


	// Animation getters / setters
	CAnimation* GetAnimation( TUInt32 slot )
	{
		return m_Animations[slot].animation;
	}
	void SetAnimation( CAnimation* anim, TUInt32 slot )
	{
		m_Animations[slot].animation = anim;  // Other animation settings remain unchanged
	}

	TFloat32 GetAnimationWeight( TUInt32 slot )
	{
		return m_Animations[slot].weight;
	}
	void SetAnimationWeight( TUInt32 slot, TFloat32 weight )
	{
		m_Animations[slot].weight = weight;
	}

	TFloat32 GetAnimationPosition( TUInt32 slot )
	{
		return m_Animations[slot].position;
	}
	void SetAnimationPosition( TUInt32 slot, TFloat32 pos )
	{
		m_Animations[slot].position = pos;
	}

	TFloat32 GetAnimationSpeed( TUInt32 slot )
	{
		return m_Animations[slot].speed;
	}
	void SetAnimationSpeed( TUInt32 slot, TFloat32 speed )
	{
		m_Animations[slot].speed = speed;
	}


	/////////////////////////////////////
	// Rendering

	// Calculate the model's absolute world transforms
	void CalculateTransforms();
	
	// Render the model from the given camera
	void Render( CCamera* camera );
	

	/////////////////////////////////////
	// Miscellaneous keyframe support 

	// Output current transforms as a keyframe to a text file
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

	// A list of animations simultaneously playing on this model (blended together)
	// Would use a dynamic list for a larger project
	SAnimationCtrl  m_Animations[NumAnimationSlots];

	// Total weight of animated bones accumulated onto a given relative transform during
	// animation blending
	TFloat32*       m_TotalWeights;

	// Relative and absolute world transforms for each node - quaternion-based, not matrices
	CQuatTransform* m_RelTransforms; // Dynamically allocated arrays
	CQuatTransform* m_Transforms;

	// Actual matrices used for rendering - calculated from absolute transform array
	CMatrix4x4*     m_Matrices;
};


} // namespace gen