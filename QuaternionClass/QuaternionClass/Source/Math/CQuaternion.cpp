/**************************************************************************************************
	Module:       CQuaternion.cpp
	Author:       Laurent Noel
	Date created: 23/06/06

	Implementation of the concrete class CQuaternion, four 32-bit floats representing a quaternion
	Slightly reordered and simplified from full maths class version for lab exercise

	Copyright 2006, University of Central Lancashire and Laurent Noel

	Change history:
		V1.0    Created 23/06/06 - LN
**************************************************************************************************/

#include "CQuaternion.h"

namespace gen
{

/*-----------------------------------------------------------------------------------------
	Quaternion multiplication
-----------------------------------------------------------------------------------------*/

// Return the quaternion result of multiplying two quaternions - non-member function
CQuaternion operator*
(
	const CQuaternion& q1,
	const CQuaternion& q2
)
{
	CQuaternion mulQ; // Final result goes in this variable...
	TFloat32 mulW;    // ...but might be easier to do the calculations...
	CVector3 mulV;    // ...in these variables first (you decide)
	
	// The formula uses dot and cross products for vectors - so you should use the
	// functions Dot and Cross for CVector3 that exist in the gen namespace. However,
	// there is a Dot function declared for the CQuaternion class too, and we don't
	// want that one. This can potentially be a problem, with the compiler not knowing
	// which version to use. If you get such a warning, fix it by explicitly using the
	// gen namespace version: gen::Dot. This disambiguates the function call from the 
	// quaternion version (whose fully qualified name is gen::CQuaternion::Dot)

	const CVector3 v1 = CVector3(q1); // Get the vector part of the first quaternion. Can also use:
	                                  //    const CVector3& v1 = q1.Vector();
	                                  // for efficiency, but read comments on CQuaternion::Vector()
	//*** Fill in rest of code here...
	const CVector3 v2 = CVector3(q2);

	mulW = q1.w * q2.w - Dot(v1, v2);
	mulV = q1.w * v2 + q2.w * v1 + Cross(v2, v1);
	mulQ.w = mulW;
	mulQ.x = mulV.x;
	mulQ.y = mulV.y;
	mulQ.z = mulV.z;



	return mulQ;
}


/*-----------------------------------------------------------------------------------------
	Vector transformation
-----------------------------------------------------------------------------------------*/

// Rotate a CVector3 by this quaternion
CVector3 CQuaternion::Rotate
(
	const CVector3& p
) const
{
	CVector3 rotP; // Result goes here
	CVector3 v = CVector3(*this); // Get vector part of this quaternion, again for efficiency use:
	                              //     const CVector3& v = Vector();
	//*** Fill in code here...

	rotP = (2 * this->w * this->w - 1) * p + 2 * (gen::Dot(v, p)) * v + 2 * this->w * (Cross(v, p));


	return rotP;
}


/*-----------------------------------------------------------------------------------------
	Construction / conversion
-----------------------------------------------------------------------------------------*/

// Construct a quaternion from a CMatrix4x4 - uses upper left 3x3 only
CQuaternion::CQuaternion
(
	const CMatrix4x4& m
)
{
	// Calculate matrix scaling
	TFloat32 scaleX = Sqrt( m.e00*m.e00 + m.e01*m.e01 + m.e02*m.e02 );
	TFloat32 scaleY = Sqrt( m.e10*m.e10 + m.e11*m.e11 + m.e12*m.e12 );
	TFloat32 scaleZ = Sqrt( m.e20*m.e20 + m.e21*m.e21 + m.e22*m.e22 );

	// Calculate inverse scaling to extract rotational values only
	GEN_ASSERT( !gen::IsZero(scaleX) && !gen::IsZero(scaleY) && !gen::IsZero(scaleZ),
				"Cannot extract rotation from singular matrix" );
	TFloat32 invScaleX = 1.0f / scaleX;
	TFloat32 invScaleY = 1.0f / scaleY;
	TFloat32 invScaleZ = 1.0f / scaleZ;

	// Calculate trace of matrix (the sum of diagonal elements)
	TFloat32 diagX = m.e00 * invScaleX; // Remove scaling
	TFloat32 diagY = m.e11 * invScaleY;
	TFloat32 diagZ = m.e22 * invScaleZ;
	TFloat32 trace = diagX + diagY + diagZ;

	// Simple method if trace is positive
	if (trace > 0.0f)
	{
		// Derive quaternion from remaining elements
		TFloat32 s = Sqrt( trace + 1.0f );
		w = s * 0.5f;
		TFloat32 invS = 0.5f / s;
		x = (m.e12*invScaleY - m.e21*invScaleZ) * invS;
		y = (m.e20*invScaleZ - m.e02*invScaleX) * invS;
		z = (m.e01*invScaleX - m.e10*invScaleY) * invS;
	}
	else
	{
		// Find largest x,y or z axis component by manipulating diagonal elts
		TFloat32 maxAxis, invMaxAxis;
		if (diagX > diagY)
		{
			if (diagX > diagZ)
			{
				maxAxis = Sqrt( diagX - diagY - diagZ + 1.0f );
				x = 0.5f * maxAxis;
				invMaxAxis = 0.5f / maxAxis;
				y = (m.e01*invScaleX + m.e10*invScaleY) * invMaxAxis;
				z = (m.e20*invScaleZ + m.e02*invScaleX) * invMaxAxis;
				w = (m.e12*invScaleY - m.e21*invScaleZ) * invMaxAxis;
			}
			else
			{
				maxAxis = Sqrt( diagZ - diagX - diagY + 1.0f );
				z = 0.5f * maxAxis;
				invMaxAxis = 0.5f / maxAxis;
				x = (m.e20*invScaleZ + m.e02*invScaleX) * invMaxAxis;
				y = (m.e12*invScaleY + m.e21*invScaleZ) * invMaxAxis;
				w = (m.e01*invScaleX - m.e10*invScaleY) * invMaxAxis;
			}
		}
		else if (diagY > diagZ)
		{
			maxAxis = Sqrt( diagY - diagZ - diagX + 1.0f );
			y = 0.5f * maxAxis;
			invMaxAxis = 0.5f / maxAxis;
			z = (m.e12*invScaleY + m.e21*invScaleZ) * invMaxAxis;
			x = (m.e01*invScaleX + m.e10*invScaleY) * invMaxAxis;
			w = (m.e20*invScaleZ - m.e02*invScaleX) * invMaxAxis;
		}
		else
		{
			maxAxis = Sqrt( diagZ - diagX - diagY + 1.0f );
			z = 0.5f * maxAxis;
			invMaxAxis = 0.5f / maxAxis;
			x = (m.e20*invScaleZ + m.e02*invScaleX) * invMaxAxis;
			y = (m.e12*invScaleY + m.e21*invScaleZ) * invMaxAxis;
			w = (m.e01*invScaleX - m.e10*invScaleY) * invMaxAxis;
		}
	}
}


/*-----------------------------------------------------------------------------------------
	Length operations
-----------------------------------------------------------------------------------------*/

// Normalise the quaternion - make it unit length as a 4-vector
void CQuaternion::Normalise()
{
	TFloat32 fNormSquared = w*w + x*x + y*y + z*z;

	if ( gen::IsZero( fNormSquared ) )
	{
		w = x = y = z = 0.0f;
	}
	else
	{
		TFloat32 fInvLength = InvSqrt( fNormSquared );
		w *= fInvLength;
		x *= fInvLength;
		y *= fInvLength;
		z *= fInvLength;
	}
}


// Return a normalised version of a quaternion (unit length as a 4-vector) - non-member version
CQuaternion Normalise
(
	const CQuaternion& quat
)
{
	TFloat32 fNormSquared = quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z;

	if ( gen::IsZero( fNormSquared ) )
	{
		return CQuaternion( 0.0f, 0.0f, 0.0f, 0.0f );
	}
	else
	{
		TFloat32 fInvLength = InvSqrt( fNormSquared );
		return CQuaternion( quat.w*fInvLength, quat.x*fInvLength,
		                    quat.y*fInvLength, quat.z*fInvLength );
	}
}


/*---------------------------------------------------------------------------------------------
	Static constants
---------------------------------------------------------------------------------------------*/

// Standard vectors
const CQuaternion CQuaternion::kZero( 0.0f, 0.0f, 0.0f, 0.0f );
const CQuaternion CQuaternion::kIdentity( 1.0f, 0.0f, 0.0f, 0.0f );


} // namespace gen
