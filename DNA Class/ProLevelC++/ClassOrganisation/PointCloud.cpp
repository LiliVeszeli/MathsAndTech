//******************************************//
// PointCloud class implementation
//
// Holds a collection of points and provides various geometric functions for them
// (Fake class for demonstration purposes only)
//******************************************//

//-------------------------------------------------------------------------------------------------
// Prologue code - standard opening code for a cpp file
//-------------------------------------------------------------------------------------------------

// Always add the cpp's own header file first, then any other files from your own project
#include "PointCloud.h"

// Add other header files with the most specific first (e.g. 3rd party libraries) and the most general purpose last
// This order of header files will help you spot header file dependencies more easily
#include <algorithm>
#include <exception>

// namespaces - prevent names in this part of the program clashing with names in other parts
namespace myapp { 
namespace geometry {



//-------------------------------------------------------------------------------------------------
// Implementation
//-------------------------------------------------------------------------------------------------

// The order of the rest of the cpp file is more open to preference since only the class programmer will see it
// Generally the same order as the header file makes sense though


// Construct with a pointer to some source data points
// Will throw std::bad_alloc if not enough memory or too many points 
template <typename T>
PointCloud<T>::PointCloud(const Vector3<T>* sourcePoints, int numPoints, bool normalise /*= false*/)
{
	mNormalised = normalise;

	// Throw an exception if user asks for too many points. Constructors can't return error messages so exceptions are the only way out on error
	if (numPoints > MAX_POINTS)  throw std::bad_alloc();

	// Reserve space in the vector first - this is a performance optimisation you should remember. A vector has a default
	// amount of memory reserved for it at first. If you keep using push_back on a vector, it periodically runs out of
	// space and has to allocate some more memory. That takes time. Reserve all the space first to stop this happening.
	// This line can throw an exception - not going to catch it here - instead just add it to the constructor comments
	mPoints.reserve(numPoints);
	
	for (int i = 0; i < numPoints; ++i)
	{
		mPoints.push_back(sourcePoints[i]);
	}
	
	if (mNormalised)
	{
		// Do something fancy here...
	}
}



// Return length of shortest path connecting all the points in the cloud (example function, not actually going to implement this)
template <typename T>
float PointCloud<T>::GetShortestPath()
{
	// Not implementing this
	return 0;
}



// Create a convex mesh of triangles that surround the point cloud (example function, not actually going to implement this)
template <typename T>
std::vector<typename PointCloud<T>::Triangle> PointCloud<T>::CreateConvexHull()
{
	std::vector<PointCloud<T>::Triangle> triangles;
	
	// Not implementing this convex hull function, let's just send back two arbritrary triangles
	triangles.push_back({5,10,20});
	triangles.push_back({3,11,14});
	
	return triangles;
}



template <typename T>
void PointCloud<T>::CalculateMeshClosure()
{
	// Not implementing this
	return;
}


//-------------------------------------------------------------------------------------------------
// Template instantiations
//-------------------------------------------------------------------------------------------------
// This is a fairly rare but rather useful technique
// With template classes we normally have to put all code in the header file. However, if we
// specifically say in a cpp file which versions of the templated classes we want, then we can have
// the usual pair of files - the class outline in the header file and implementation in the cpp.
// The limitation is that only the given types (float and double here) are supported.

template struct Vector3<float>;
template class PointCloud<float>;

template struct Vector3<double>;
template class PointCloud<double>;


} } // Namespaces
