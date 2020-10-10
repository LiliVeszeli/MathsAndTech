/*******************************************
	
	Animation.h

	Animation class declaration
	Holds a single animation for use by
	QModel objects

********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CQuatTransform.h"

namespace gen
{

// Forward declaration of CAnimation so it can be used before it is fully declared
class CAnimation;

/*---------------------------------------------------------------------------------------------
	Animation Support Types
---------------------------------------------------------------------------------------------*/

// Each animation being played is controlled with a SAnimationCtrl structure
struct SAnimationCtrl
{
	CAnimation* animation; // Pointer to animation being played
	TFloat32    position;  // Current position in animation (0 to animation length in seconds)
	TFloat32    speed;     // Current speed of animation playback
	                       // (1.0 = normal speed, 2.0 double speed, 0.5 half speed etc)
	TFloat32    weight;    // Overall weight of this animation when blending with other animations
	bool        looping;   // Is the animation looping - if not it is removed when it ends
};


// Animation class
class CAnimation
{
/*-----------------------------------------------------------------------------------------
	Constructors/Destructors
-----------------------------------------------------------------------------------------*/
public:
	// Animation constructor
	CAnimation
	(
		const string& fileName,
		TUInt32       numKeyFrames,
		TFloat32      keyFramesPerSecond
	);

	~CAnimation();

private:
	// Disallow use of copy constructor and assignment operator (private and not defined)
	CAnimation( const CAnimation& );
	CAnimation& operator=( const CAnimation& );


/*-----------------------------------------------------------------------------------------
	Public interface
-----------------------------------------------------------------------------------------*/
public:

	/////////////////////////////////////
	// Getters

	TFloat32 GetLength()
	{
		return m_Length;
	}

	CVector3 GetVelocity()
	{
		return m_AvgVelocity;
	}


	/////////////////////////////////////
	// Interpolation 

	// Linearly interpolate two keyframes given an animation control. Add the resultant transform
	// to the given output transform using the weights of each bone - also update the total weights
	// for each output transform
	void AddKeyFrameLerp
	(
		const SAnimationCtrl& ctrl,
		CQuatTransform*       transforms,
		TFloat32*             totalWeights
	);

	// Spherical linearly interpolate two keyframes given an animation control. Add the resultant
	// transform to the given output transform using the weights of each bone - also update the
	// total weights for each output transform
	void AddKeyFrameSlerp
	(
		const SAnimationCtrl& ctrl,
		CQuatTransform*       transforms,
		TFloat32*             totalWeights
	);


	/////////////////////////////////////
	// Keyframe reading

	// Read transforms from a text file into a given keyframe
	void ReadKeyFrame
	(
		int           keyFrame,
		const string& fileName
	);


/*-----------------------------------------------------------------------------------------
	Private interface
-----------------------------------------------------------------------------------------*/
private:
	
	/*---------------------------------------------------------------------------------------------
		Data
	---------------------------------------------------------------------------------------------*/

	// Number of key frames per second in this animation - assuming this is constant for each
	// animation, a more flexible scheme would be appropriate in a larger app
	const TFloat32 m_KeyFramesPerSecond;

	TFloat32  m_Length;       // Length of the animation in seconds
	TUInt32   m_NumKeyFrames; // Number of keyframes in the animation

	CVector3  m_AvgVelocity;  // Average linear velocity (units/second) over the animation

	TUInt32   m_NumBones;     // Number of bones/nodes in the animation
	TFloat32* m_BoneMasks;    // Bone mask for each bone - the weight given to the animation of
	                          // this bone. If 0 then that bone is not animated (and stores no data)

	// Define a set of keyframes for a single node
	// Would use a dynamic list for a larger project
	static const int MaxKeyFrames = 10;
	typedef CQuatTransform TKeyFrameSet[MaxKeyFrames];

	// Sets of keyframes to use for transforms
	TKeyFrameSet*   m_KeyFrameSets;
};


} // namespace gen