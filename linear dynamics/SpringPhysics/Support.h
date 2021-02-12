//-----------------------------------------------------
// Support.h
//   Support classes & helper functions for TL-Engine
//-----------------------------------------------------

#ifndef SUPPORT_H_INCLUDED
#define SUPPORT_H_INCLUDED

#include <TL-Engine.h>
using namespace tle;


//------------------------------------------
// Picking Functions
//------------------------------------------

// Convert a world point into a screen pixel given a camera (which cannot be parented)
// Returns pixel position x and y components via pointer parameters
// The function returns false if the given point is behind the camera
bool PixelFromWorldPoint( CVector3& worldPoint, ICamera* camera, I3DEngine* engine, int* x, int* y  );


// Calculate the world coordinates of a point on the near clip plane corresponding to given 
// x and y pixel coordinates using this camera and near clip distance
CVector3 WorldPointFromPixel( int x, int y, ICamera* camera, float nearClip, I3DEngine* engine );


#endif