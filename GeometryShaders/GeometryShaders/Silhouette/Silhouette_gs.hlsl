//--------------------------------------------------------------------------------------
// Silhouette Geometry Shader
//--------------------------------------------------------------------------------------
// In this example each triangle has special data provided about its adjacent triangles
// Geometry shaders get access to this additional information. Here we compare the face
// normal of each triangle with the face normal of its neighbours. Where the face normal
// points towards us on one triangle, but away from us on the other triangle, then the
// edge connecting those triangles must be part of the silhouette.
// 
// This geometry shader inputs triangles and outputs lines. We could expand those lines
// into polygons to draw brush strokes for each edge or something arty like that...

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------


///////////////////////////////////////////
// Helper function for silhouette shader

// Return a face normal, given three points in the face. Uses the cross product of two edges formed by the points
float3 FaceNormalFromPoints(float3 A, float3 B, float3 C)
{
	//**** MISSING - write this helper function. A similar process was used in previous geometry shader exercise
    float3 faceEdgeA = A - B;
    float3 faceEdgeB = C - B;
    float3 faceNormal = normalize(cross(faceEdgeA, faceEdgeB));
	
    return faceNormal;
}



///////////////////////////////////////////
// Main shader

//**** MISSING LINE - see previous exercise for syntax. Work out the value: this shader looks at a triangle, and outputs a line for each edge of that triangle that is a silhouette edge
[maxvertexcount(6)]
void main
(
	// First parameter is the input - specified here as triangles with adjacency - 6 vertices each (see lecture notes)
	triangleadj BasicVertex input[6], 
	 
	// Second parameter is used for output - here creating a stream of *lines* to replace the input triangles. This data will be input into the lighting pixel shader (see last exercise)
	inout LineStream<LightingPixelShaderInput> outStrip
)
{
	LightingPixelShaderInput output; // Used to build output vertices

	// Calculate the face normal for the current triangle - using a helper function this time
	// Note that the even vertices form the original triangle. The odd vertices are adjacent to it (see lecture notes)
	float3 faceNormal = FaceNormalFromPoints( input[0].position.xyz,
	                                          input[2].position.xyz,
	                                          input[4].position.xyz );

	// First check if this face is pointing towards camera. This is actually the back-face culling test - when using 
	// geometry shaders it is not too uncommon to perform this test ourselves
	float3 cameraDir = gCameraPosition - input[0].position.xyz;
	if (dot( cameraDir, faceNormal ) >= 0)
	{		
		// Iterate through the 3 adjacent triangles, loop counter represents first vertex of each adjacent triangle
		for (uint adj0 = 0; adj0 < 6; adj0 += 2)
		{
			// Get other two vertices of adjacent triangle (see lecture notes for adjacency data layout)
			uint adj1 = adj0 + 1;
			uint adj2 = (adj0 + 2) % 6;
			
			// Get face normal of adjacent triangle
			float3 adjNormal = FaceNormalFromPoints( input[adj0].position.xyz,
			                                         input[adj1].position.xyz,
			                                         input[adj2].position.xyz );
			                              
			// If adjacent face is pointing *away from* the camera, then the edge to this adjacent 
			// triangle is a silhouette edge
			cameraDir = gCameraPosition - input[adj0].position.xyz;
			if (/**** MISSING - test if adjacent face is pointing away from camera direction, similar to test a few lines above this */ dot(cameraDir, adjNormal) <= 0)
			{
				// Output this silhouette edge between the original and adjacent triangle (refer to previous exercise for details)
				// Start vertex of line
				output.worldPosition     = input[adj0].position;
				output.projectedPosition = mul(gViewProjectionMatrix, float4(input[adj0].position, 1));
				output.worldNormal       = input[adj0].normal;
				output.uv                = input[adj0].uv;
				outStrip.Append( output );

				// End vertex of line
				output.worldPosition     = input[adj2].position;
				output.projectedPosition = mul(gViewProjectionMatrix, float4(input[adj2].position, 1));
				output.worldNormal       = input[adj2].normal;
				output.uv                = input[adj2].uv;
				outStrip.Append( output );
			
				// The output is a line strip (a joined up line), but the silhouette edges may not join up, so start
				// a new strip after each line is output
				outStrip.RestartStrip();
			}
		} 
	}
}
