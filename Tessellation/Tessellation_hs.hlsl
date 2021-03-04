//**************************************************************************************
// Tessellation Hull Shader (and patch constant function)
//**************************************************************************************
// The Hull Shader reads in a set of "patches", surfaces defined by "control points". The hull shader is called once for each control point, and its job is to
// do any kind of manipulation or calculation of the control points required *prior* to the tessellation
// A patch is some kind of surface, which starts as a line, triangle or quad, but which will be tessellated and curved/bent into any shape we wish based on the
// control points. Exactly how the control points define the shape of the patch is up to the programmer. For example the control points might be used to take a
// quad and manipulate it as a Bezier spline. However for the case of displacement mapping we won't use splines, the control points are just the vertices of each
// triangle, and we're not going to manipulate them prior to tessellation, so there is little for this hull shader to do yet. However, we need to define a hull
// shader in any case - the most important part being the sequence of attributes that are written before the shader itself:
//

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

[domain("tri")]                  // The input before the tessellation is triangles
[partitioning("fractional_odd")] // Tessellation style to use - can use "fractional_even", "fractional_odd", "integer" or "pow2"
[outputtopology("triangle_cw")]  // We want the tessellated output to be triangles too
[outputcontrolpoints(/*MISSING!*/)]         // Not using splines or similar (see lecture) in this case the input and output "control points" are just the vertices of each triangle
[patchconstantfunc("DistanceTessellation")] // Name of the "patch constant" function used to select the tessellation factors etc. (see the function after this one)
[maxtessfactor(50.0)]                       // Maximum tessellation factor that the constant function (named on the line above) will select
HullShaderOutput main
(
	InputPatch<HullShaderInput, 3> inputPatch, // The input patch is an array of control points (up to 32 per patch), here simply the 3 vertices of each triangle
	uint pointID : SV_OutputControlPointID     // This function is called once for each control point, this integer counts from 0 to indicate which point we are on (0, 1, 2 for a triangle)
)
{
    HullShaderOutput output;
    
    // Just copy current control point to the output. The Domain Shader will do the displacement mapping after receiving the output of the tessellation stage
    output.worldPosition = inputPatch[pointID].worldPosition;
    output.worldNormal   = inputPatch[pointID].worldNormal;
    output.worldTangent  = /*MISSING!*/;
    output.uv            = inputPatch[pointID].uv;

	// Calculate the tessellation factor for the opposite edge from this point, result will be used in the patch constant function below
	// This is the solution to watertight seams when varying tessellation by distance: for each edge, we find the furthest of the two points to
	// the camera and set the tessellation factor based on that distance. Of course, the patch on either side of the edge will agree on where the
	// two points in the edge are and which is furthest from the camera, so both patches will choose the same edge tessellation ==> watertight seams
	// Technical point: this code would seem better in the constant function below where the tessellation factors are actually output. However, it
	// doesn't seem to work correctly there - maybe there's some vertex reordering (or I just got it wrong!). In any case, this is the location 
	// suggested by NVidia and Epic, although there is no documentation to back it up... This kind of guesswork is common with newer features...
	const uint oppID1 = pointID < 2 ? pointID + 1 : 0; // Find the two points making up the edge opposite to this point in the triangle
	const uint oppID2 = oppID1  < 2 ? oppID1  + 1 : 0; // --"--
	float maxDist = max(distance(gCameraPosition, inputPatch[oppID1].worldPosition), 
		                distance(gCameraPosition, inputPatch[oppID2].worldPosition));  // Max distance from the two points to the camera
	const float distanceRange = gMinTessellationDistance - gMaxTessellationDistance;
	output.oppositeEdgeFactor = lerp( gMaxTessellation, gMinTessellation,
		                              sqrt(saturate((maxDist - gMaxTessellationDistance) / distanceRange)) ); // Tessellation factor in range required

    return output;
}


// The "patchconstant function" used for the hull shader above. All hull shaders require such a function. It is called once for each patch (rather than once
// for each control point) and sets up any values that are constant for the entire patch. In this example it is called once for each triangle in our mesh.
// At a minimum the function must specify the "Tessellation factors": these determine how finely the patches (triangles) will be tessellated
// For a triangle patch, we specify one tessellation factor for each edge (how many times the edges will be split), and one factor for the inside of the
// triangle (how many extra triangles will be generated in the interior of the triangle)
// Using the same tessellation factor for everything is simplest and will create a nice regular tessellation. However, if you are using any kind of varying
// tessellation (e.g. tessellation that decreases with distance), then the edges between triangles of different tessellation will need to have their factors
// matched or you will see cracks in the geometry
PatchConstantOutput DistanceTessellation
(
	// This function can read the vertex shader input patches and/or the updated patches that were output from the hull shader.
	// Here we only read the output of the hull shader to help with calculating distance-based tessellation factors
	OutputPatch<HullShaderOutput, 3> outputPatch  
)
{
    PatchConstantOutput output;

	// Using tessellation factors calculated in the hull shader above - see the comment there
	// Tech detail: The edge referred to is the one opposite to the array index, so EdgeTess[0] refers to the edge from point 1 to point 2
    output.edgeFactor[0] = outputPatch[0].oppositeEdgeFactor; // Tessellation for triangle edge
    output.edgeFactor[1] = outputPatch[1].oppositeEdgeFactor;
    output.edgeFactor[2] = outputPatch[2].oppositeEdgeFactor;
    output.insideFactor  = min( min( output.edgeFactor[0], output.edgeFactor[1]), output.edgeFactor[2] ); // Tesselation for triangle interior

    return output;
}

