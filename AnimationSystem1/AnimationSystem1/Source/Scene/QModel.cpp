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

	for (TUInt32 anim = 0; anim < NumAnimationSlots; ++anim)
	{
		m_Animations[anim].animation = 0;
	}

	// Allocate space for transforms and matrices
	TUInt32 numNodes = m_Mesh->GetNumNodes();
	m_RelTransforms = new CQuatTransform[numNodes];
	m_TotalWeights = new TFloat32[numNodes];
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
}


CQModel::~CQModel()
{
	delete[] m_Matrices;
	delete[] m_Transforms;
	delete[] m_TotalWeights;
	delete[] m_RelTransforms;
}


//-----------------------------------------------------------------------------
// Animation Support
//-----------------------------------------------------------------------------

// Play a new animation at the given slot in the animation list for a model.
// A single model can play several animations simultaneously - i.e several slots.
// All animations for a model are blended together to produce the final result.
// Pass an animation of 0 to remove the animation in that slot.
// Optionally pass whether to loop the animation, its weight (compared to other animations playing
// on the same model), initial position (0->1, within the animation) and speed multiplier
void CQModel::PlayAnimation( CAnimation* anim, TUInt32 slot, bool looping /*= false*/,
                             TFloat32 weight /*= 1.0f*/, TFloat32 pos /*= 0.0f*/, 
							 TFloat32 speed /*= 1.0f*/ )
{
	m_Animations[slot].animation = anim;
	m_Animations[slot].looping = looping;
	m_Animations[slot].weight = weight;
	m_Animations[slot].position = pos;
	m_Animations[slot].speed = speed;
}


// Update all current animations by the given amount of time
void CQModel::UpdateAnimations( TFloat32 frameTime )
{
	// For each animation slot
	for (TUInt32 anim = 0; anim < NumAnimationSlots; ++anim)
	{
		// If an animation is being played in this slot
		if (m_Animations[anim].animation)
		{
			// Update animation position (in time)
			m_Animations[anim].position += frameTime * m_Animations[anim].speed;

			// Move the model by the weighted velocity of this animation - regenerating the
			 //motion extracted from the animation at creation time
			TFloat32 animMove = frameTime * m_Animations[anim].speed * m_Animations[anim].weight;
			CVector3 vel = m_Animations[anim].animation->GetVelocity();
			CVector3 scaleVel(m_RelTransforms[0].scale.x * vel.x, m_RelTransforms[0].scale.y * vel.y, 
			                  m_RelTransforms[0].scale.z * vel.z);
			m_RelTransforms[0].pos += scaleVel * animMove;

			// Check if position has exceeded animation length
			TFloat32 animLength = m_Animations[anim].animation->GetLength();
			if (m_Animations[anim].position < 0 || m_Animations[anim].position >= animLength)
			{
				// If it is a looping animation
				if (m_Animations[anim].looping)
				{
					// Set the position back to a valid value (loop it round)
					m_Animations[anim].position = fmodf( m_Animations[anim].position, animLength );
					if (m_Animations[anim].position < 0)
					{
						m_Animations[anim].position = animLength - m_Animations[anim].position;
					}
				}
				else
				{
					// Remove the animation
					m_Animations[anim].animation = 0;
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------

// Calculate the model's absolute world matrices
void CQModel::CalculateTransforms()
{
	// Intialise total bone weights from all animations
	for (TUInt32 node = 0; node < m_Mesh->GetNumNodes(); ++node)
	{
		m_TotalWeights[node] = 0.0f;
	}

	// For each animation slot
	for (TUInt32 anim = 0; anim < NumAnimationSlots; ++anim)
	{
		// If an animation is being played in this slot
		if (m_Animations[anim].animation)
		{
			// Accumulate the effect of this animation using lerp
			// Update the total weights accumulated onto each bone
			m_Animations[anim].animation->AddKeyFrameLerp( m_Animations[anim], m_RelTransforms,
			                                               m_TotalWeights );
		}
	}

	// After accumulating the weighted animations, divide each bone's final transform down by 
	// the total bone weights. [Note how the above process is similar to vertex skinning]
	for (TUInt32 node = 0; node < m_Mesh->GetNumNodes(); ++node)
	{
		if (m_TotalWeights[node] != 0.0f)
		{
			m_RelTransforms[node] /= m_TotalWeights[node];
		}
	}


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
// Miscellaneous keyframe support 
//-----------------------------------------------------------------------------

// Output current transforms to a text file
void CQModel::WriteTransforms
(
	const string& fileName
)
{
	// Open file stream for writing
	fstream fileOut( fileName.c_str(), ios::out );

	// Output number of nodes
	fileOut << m_Mesh->GetNumNodes() << " Nodes" << endl << endl;

	// Output each node
	for (TUInt32 node = 0; node < m_Mesh->GetNumNodes(); ++node)
	{
		const SMeshNode& meshNode = m_Mesh->GetNode( node );

		const CQuatTransform& t = m_RelTransforms[node];
		fileOut << "Node " << node << " - " << meshNode.name << endl;
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