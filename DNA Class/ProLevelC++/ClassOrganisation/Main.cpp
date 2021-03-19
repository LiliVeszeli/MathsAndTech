//******************************************//
// Main entry point and sample class usage
//******************************************//

#include "PointCloud.h"


namespace myapp {

int Run()
{
	geometry::Vector3<float> initPoints[] = { {1.0f, 2.5f, -1.5f},  {3.2f, 4.3f, -0.5f},  {0.2f, 1.7f, 1.6f} };
	
	// Point cloud constructor can throw exceptions. Don't ignore that, catch the error and deal with it
	try 
	{
		geometry::PointCloud<float> myPointCloud(initPoints, 3);

		// Standard getter/setter
		myPointCloud.SetPoint(1, {3.2f, 1.5f, 5.6f});
		auto p = myPointCloud.GetPoint(1);

		// Dangerous but convenient/fast alternative accessor - direct access to the point cloud internal data
		p = myPointCloud.Point(1);
		myPointCloud.Point(1) = {7.3f, 1.4f, 4.6f};


		auto triangles = myPointCloud.CreateConvexHull();

		return 0;
	}
	catch (...)
	{
		return -1;
	}
}


} // Namespaces



// Main can't be in a namespace, so it just calls a Run function in the myapp namespace
// You could use a class myapp instead of a namespace to do a similar thing
int main()
{
	return myapp::Run();
}