//-----------------------------------------------------
// Support.cpp
//   Support classes & helper functions for TL-Engine
//-----------------------------------------------------

#include "BaseMath.h"
#include "CMatrix4x4.h"
#include "CVector3.h"
using namespace gen;

#include "Support.h"


//------------------------------------------
// Picking Functions
//------------------------------------------

// Convert a world point into a screen pixel given a camera (which cannot be parented)
// Returns pixel position x and y components via pointer parameters
// The function returns false if the given point is behind the camera
bool PixelFromWorldPoint( CVector3& worldPoint, ICamera* camera, I3DEngine* engine, int* x, int* y  )
{
	// Calculate horizontal and vertical field of view to match TL-Engine internal settings
	const float fovMin = kfPi / 3.4f; // Default TL FOV for minimum of width or height
	float fovX;
	float fovY;
	float width = static_cast<float>(engine->GetWidth());
	float height = static_cast<float>(engine->GetHeight());
	if (width > height)
	{
		fovY = fovMin;
		fovX = ATan( width * Tan(fovY * 0.5f) / height ) * 2.0f;
	}
	else
	{
		fovX = fovMin;
		fovY = ATan( height * Tan(fovX * 0.5f) / width ) * 2.0f;
	}

	// Transform to camera space
	float cameraMatrixElts[16];
	camera->GetMatrix( cameraMatrixElts );
	CMatrix4x4 viewMatrix = InverseAffine( CMatrix4x4(cameraMatrixElts) );
	CVector3 cameraPoint = viewMatrix.TransformPoint( worldPoint );

	// Projection matrix (equivalent)
	cameraPoint.x /= Tan( fovX * 0.5f );
	cameraPoint.y /= Tan( fovY * 0.5f );

	// Perspective divide (test if behind camera)
	if (cameraPoint.z <= 0)
	{
		return false;
	}
	cameraPoint.x /= cameraPoint.z;
	cameraPoint.y /= cameraPoint.z;

	// Convert to pixel dimensions
	*x = static_cast<int>((cameraPoint.x + 1.0f) * width * 0.5f);
	*y = static_cast<int>((1.0f - cameraPoint.y) * height * 0.5f);

	return true;
}


// Calculate the world coordinates of a point on the near clip plane corresponding to given 
// x and y pixel coordinates using this camera and near clip distance
CVector3 WorldPointFromPixel( int x, int y, ICamera* camera, float nearClip, I3DEngine* engine )
{
	// Calculate horizontal and vertical field of view to match TL-Engine internal settings
	const float fovMin = kfPi / 3.4f; // Default TL FOV for minimum of width or height
	float fovX;
	float fovY;
	float width = static_cast<float>(engine->GetWidth());
	float height = static_cast<float>(engine->GetHeight());
	if (width > height)
	{
		fovY = fovMin;
		fovX = ATan( width * Tan(fovY * 0.5f) / height ) * 2.0f;
	}
	else
	{
		fovX = fovMin;
		fovY = ATan( height * Tan(fovX * 0.5f) / width ) * 2.0f;
	}

	// Reverse procedure in function above
	CVector3 cameraPoint;
	cameraPoint.x = static_cast<float>(x) / (width * 0.5f) - 1.0f;
	cameraPoint.y = 1.0f - static_cast<float>(y) / (height * 0.5f);
	
	cameraPoint.x *= nearClip;
	cameraPoint.y *= nearClip;
	cameraPoint.z = nearClip;

	cameraPoint.x *= Tan( fovX * 0.5f );
	cameraPoint.y *= Tan( fovY * 0.5f );

	float cameraMatrixElts[16];
	camera->GetMatrix( cameraMatrixElts );
	CMatrix4x4 cameraMatrix = CMatrix4x4(cameraMatrixElts);
	CVector3 worldPoint = cameraMatrix.TransformPoint( cameraPoint );

	return worldPoint;
}
