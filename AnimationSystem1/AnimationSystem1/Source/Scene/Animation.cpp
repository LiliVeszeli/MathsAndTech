/*******************************************
	
	Animation.cpp

	Animation class implementation
	Holds a single animation for use by
	QModel objects

********************************************/

#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "Animation.h"

namespace gen
{

// Folder for all animation files
static const string MediaFolder = "Media\\";


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

// Animation constructor
CAnimation::CAnimation
(
	const string& fileName,
	TUInt32       numKeyFrames,
	TFloat32      keyFramesPerSecond
) : m_KeyFramesPerSecond( keyFramesPerSecond )
{
	// Calculate keyframe / length information from parameters. This information would be
	// better stored in the animation file, keeping it simple here
	m_NumKeyFrames = numKeyFrames;
	m_Length = (numKeyFrames - 1) / keyFramesPerSecond;

	m_AvgVelocity = CVector3::kZero;

	// Get number of bones/nodes from animation mask file ("AnimationNameMask")
	stringstream frameFile;
	frameFile << MediaFolder << fileName << "Mask.txt";
	fstream fileIn( frameFile.str().c_str(), ios::in );
	string tmpString;
	fileIn >> m_NumBones >> tmpString;

	// Allocate space for bone masks
	m_BoneMasks = new TFloat32[m_NumBones];

	// Read the bone masks for each bone
	char tmpChar;
	int tmpInt;
	for (TUInt32 bone = 0; bone < m_NumBones; ++bone)
	{
		fileIn >> tmpString >> tmpInt >> tmpChar >> tmpString >> m_BoneMasks[bone];
	}
	fileIn.close();


	// Allocate space for keyframes
	m_KeyFrameSets = new TKeyFrameSet[m_NumBones];

	// Read each keyframe file. Each key frame is in a separate file (again a single animation
	// file would be better). Naming convention is AnimationNameXX where XX is a two digit number
	// starting at 00
	for (TUInt32 keyFrame = 0; keyFrame < m_NumKeyFrames; ++keyFrame)
	{
		frameFile.str("");
		frameFile << MediaFolder << fileName 
		          << char('0' + keyFrame / 10) << char('0' + keyFrame % 10) << ".txt";
		ReadKeyFrame( keyFrame, frameFile.str() );
	}

	// Linear motion extraction
	// Get starting and ending position (bone 2 is root frame here)
	CVector3 pos0 = m_KeyFrameSets[2][0].pos; 
	CVector3 pos1 = m_KeyFrameSets[2][m_NumKeyFrames - 1].pos;
	 
	// Get average motion per frame
	CVector3 avgMotion = (pos1 - pos0) / static_cast<TFloat32>(m_NumKeyFrames - 1);
	
	// Calculate average velocity (per second) for actual model movement
	m_AvgVelocity = avgMotion * keyFramesPerSecond;

	// Remove averaged motion from animation
	CVector3 subtractMotion = CVector3::kOrigin;
	for (TUInt32 keyFrame = 0; keyFrame < m_NumKeyFrames; ++keyFrame)
	{
		m_KeyFrameSets[2][keyFrame].pos -= subtractMotion;
		subtractMotion += avgMotion;
	}
}

// Animation destructor
CAnimation::~CAnimation()
{
	delete[] m_KeyFrameSets;
}


//-----------------------------------------------------------------------------
// Interpolation 
//-----------------------------------------------------------------------------

// Linearly interpolate (with normalisation) two keyframes given an animation control. Add the
// resultant transform to the given output transform using the weights of each bone - also update
// the total weights for each output transform
void CAnimation::AddKeyFrameLerp
(
	const SAnimationCtrl& ctrl,
	CQuatTransform*       transforms,
	TFloat32*             totalWeights
)
{
	// Calculate current keyframe position (as a floating point value)
	TFloat32 floatKeyFrames = static_cast<float>(m_NumKeyFrames - 1);
	TFloat32 aniPos = floatKeyFrames * ctrl.position / m_Length;
	aniPos = fmodf( aniPos, floatKeyFrames );  // Deal with "out of range" postions

	// Covert this value into the before and after frame and the interpolation value
	TUInt32 frame1 = static_cast<int>(aniPos);
	TUInt32 frame2 = frame1 + 1;

	// Calculate interpolation value - from 0 -> 1 between the two frames
	TFloat32 t = aniPos - frame1;

	// For each bone/node in the animation..
	++transforms;
	++totalWeights;
	for (TUInt32 bone = 1; bone < m_NumBones; ++bone)
	{
		// Combine overall animation weight with individual bone weight
		TFloat32 weight = ctrl.weight * m_BoneMasks[bone];

		// Ignore bones with no weighting
		if (weight != 0.0f)
		{
			// Calculate interpolated transform between the two keyframes
			CQuatTransform qt;
			Slerp( m_KeyFrameSets[bone][frame1], m_KeyFrameSets[bone][frame2], t, qt );

			// If this is the first animation accumulated onto this bone...
			if (*totalWeights == 0.0f)
			{
				// Replace default bone transform with this transform
				*transforms = qt * weight;
				*totalWeights = weight;
			}
			else
			{
				// Add this transform with its bone mask weighting onto the output transform
				*transforms += qt * weight;
				*totalWeights += weight;
			}
		}
		++transforms;
		++totalWeights;
	}
}


// Spherical linearly interpolate two keyframes given an animation control. Add the resultant
// transform to the given output transform using the weights of each bone - also update the
// total weights for each output transform
void CAnimation::AddKeyFrameSlerp
(
	const SAnimationCtrl& ctrl,
	CQuatTransform*       transforms,
	TFloat32*             totalWeights
)
{
	// Calculate current keyframe position (as a floating point value)
	TFloat32 floatKeyFrames = static_cast<float>(m_NumKeyFrames - 1);
	TFloat32 aniPos = floatKeyFrames * ctrl.position / m_Length;
	aniPos = fmodf( aniPos, floatKeyFrames );  // Deal with "out of range" postions

	// Covert this value into the before and after frame and the interpolation value
	TUInt32 frame1 = static_cast<int>(aniPos);
	TUInt32 frame2 = frame1 + 1;

	// Calculate interpolation value - from 0 -> 1 between the two frames
	TFloat32 t = aniPos - frame1;

	// For each bone/node in the animation..
	++transforms;
	++totalWeights;
	for (TUInt32 bone = 1; bone < m_NumBones; ++bone)
	{
		// Combine overall animation weight with individual bone weight
		TFloat32 weight = ctrl.weight * m_BoneMasks[bone];

		// Ignore bones with no weighting
		if (weight != 0.0f)
		{
			// Calculate interpolated transform between the two keyframes
			CQuatTransform qt;
			Slerp( m_KeyFrameSets[bone][frame1], m_KeyFrameSets[bone][frame2], t, qt );

			// If this is the first animation accumulated onto this bone...
			if (*totalWeights == 0.0f)
			{
				// Replace default bone transform with this transform
				*transforms = qt * weight;
				*totalWeights = weight;
			}
			else
			{
				// Add this transform with its bone mask weighting onto the output transform
				*transforms += qt * weight;
				*totalWeights += weight;
			}
		}
		++transforms;
		++totalWeights;
	}
}


//-----------------------------------------------------------------------------
// Keyframe reading
//-----------------------------------------------------------------------------

// Read transforms from a text file into a given keyframe
void CAnimation::ReadKeyFrame
(
	int           keyFrame,
	const string& fileName
)
{
	// Open file stream for reading
	fstream fileIn( fileName.c_str(), ios::in );

	// Read and check number of bones
	string tmpString;
	int numBones;
	fileIn >> numBones >> tmpString;
	if (numBones != m_NumBones)
	{
		return;
	}

	// Read each node
	char tmpChar;
	int tmpInt;
	for (TUInt32 bone = 0; bone < m_NumBones; ++bone)
	{
		const CQuatTransform& t = m_KeyFrameSets[bone][keyFrame];
		fileIn >> tmpString >> tmpInt >> tmpChar >> tmpString;
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



} // namespace gen