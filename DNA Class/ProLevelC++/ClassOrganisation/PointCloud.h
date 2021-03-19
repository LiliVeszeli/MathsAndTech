//******************************************//
// PointCloud class definition
//
// Holds a collection of points and provides
// various geometric functions for them
// (Fake class for demonstration purposes only)
//******************************************//

//-------------------------------------------------------------------------------------------------
// Prologue code - standard opening code for a header file
//-------------------------------------------------------------------------------------------------

// Header guard - prevent this header files from being included more than once
#ifndef _POINT_CLOUD_H_INCLUDED_
#define _POINT_CLOUD_H_INCLUDED_

#include <vector> // Put between header guard and namespaces

// namespaces - prevent names in this part of the program clashing with names in other parts
namespace myapp { 
namespace geometry {



//-------------------------------------------------------------------------------------------------
// General definitions - not class-specific
//-------------------------------------------------------------------------------------------------

// Vector3 is templated based on the type of number the point xyz values use (e.g. float, double)
// Obviously this would better have its own cpp/header file, but putting it here for simplicity
template <typename T>
struct Vector3
{
	T x, y, z;
};



//-------------------------------------------------------------------------------------------------
// Class definition
//-------------------------------------------------------------------------------------------------

// The PointCloud class is templated in to the type its Vector3 contents use (see above)
template <typename T>
class PointCloud
{

//-------------------------------------------------------------------------------------------------
// Public Class types
//-------------------------------------------------------------------------------------------------
public:
	// Indicates three points in the point cloud that form a triangle
	// As a public class member, this type is accessible by anyone like this for example:
	//    PointCloud<float>::Triangle tri = {5, 15, 25};
	struct Triangle
	{
		int index0, index1, index2;
	};


//-------------------------------------------------------------------------------------------------
// Constructors
//-------------------------------------------------------------------------------------------------
public:
	// Disable default PointCloud constructor. Must provide source data to create a point cloud (design decision, could have allowed it)
	PointCloud() = delete;


	// Construct with a pointer to some source data points
	// Will throw std::bad_alloc if not enough memory (really important to understand and explain what your functions do on failure)
	PointCloud(const Vector3<T>* sourcePoints, int numPoints, bool normalise = false);
	

	// Copy constructor - copy the points from another point cloud
	// Using default copy constructor - i.e. compiler will make the code for us - a shallow copy of member data (not appropriate if we have pointers in our member data)
	// This is code that would use this constructor, creating a new object by copying an existing one:
	//    PointCloud<float> newCloud = oldCloud; 
	PointCloud(const PointCloud<T>& sourcePointCloud) = default;


	// Copy assignment operator. Similar to the above, just slightly different use case. Again let the compiler create the code
	// This is code that would use this constructor, overwriting an object with a copy of an existing one:
	//    PointCloud<float> cloud1(sourcePoints1);
	//    PointCloud<float> cloud2(sourcePoints2);
	//    cloud1 = cloud2; // <-- Copy assignment
	PointCloud& operator=(const PointCloud<T>& sourcePointCloud) = default;


	// Not providing move constructors, an advanced but important topic, because the complexity would distract from the exercise.


//-------------------------------------------------------------------------------------------------
// Data access (getters setters)
//-------------------------------------------------------------------------------------------------
public:
	bool IsNormalised()  { return mNormalised; }
	
	Vector3<T> GetPoint(int index)
	{
		return mPoints[index];
	}
	
	void SetPoint(int index, const Vector3<T>& newPoint)
	{
		mPoints[index] = newPoint;
	}

	
	// Alternative to the getter/setter pair above - allows direct access to points in the vector using a non-constant reference
	// Convenient & dangerous - only recommended for simple classes (e.g. getting the position out of a matrix) or for performance reasons
	Vector3<T>& Point(int index)
	{
		return mPoints[index];
	}


//-------------------------------------------------------------------------------------------------
// Public interface (the main features of this class)
//-------------------------------------------------------------------------------------------------
public:

	// Return length of shortest path connecting all the points in the cloud (example function, not actually going to implement this)
	float GetShortestPath();

	// Create a convex mesh of triangles that surround the point cloud (example function, not actually going to implement this)
	std::vector<Triangle> CreateConvexHull();




//------------ End of public interface - the part that other people use. Everything below is private, things needed to implement the class




//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
private:
	// A type only used in the implementation (doesn't matter what it means, just an example)
	// In practice you could put this at the top of the cpp file instead and hide it from the user of the class
	enum class MeshClosure
	{
		Open,
		Closed,
		Undecided,
	};
	
	
//-------------------------------------------------------------------------------------------------
// Private member functions (functions needed to implement the class, but not available publicly)
//-------------------------------------------------------------------------------------------------
private:
	// Example function, meaning irrelevant. Something the class user doesn't need to know about but is needed to implement the class
	void CalculateMeshClosure();


//-------------------------------------------------------------------------------------------------
// Private member data
//-------------------------------------------------------------------------------------------------
private:
	static const int MAX_POINTS = 4000000; // A static const member has the same value for all PointClouds

	// Points in this point cloud
	std::vector<Vector3<T>> mPoints;

	// Rescale point cloud to be in 0->1 range
	bool mNormalised;

	// Some additional data required for implementation (meaning unimportant, just an example)
	std::vector<MeshClosure> mClosures;
};


} } // Namespaces

#endif // Header guard
