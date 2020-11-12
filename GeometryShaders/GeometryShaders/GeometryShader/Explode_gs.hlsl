//--------------------------------------------------------------------------------------
// Polygon Explode Geometry Shader
//--------------------------------------------------------------------------------------
// Expands each polygon outwards along its face normal, this wouldn't be possible in 
// the vertex shader because it needs to work on a polygon at a time rather than one
// vertex at a time

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

[maxvertexcount(6)]  // First specify maximum number of vertices *output* by the shader - this particular shader doesn't add any new vertices
void main
(
	// Note the brackets above - these are the parameters to the function spread out over a couple of lines
	// First parameter is the input - specified here as a triangle - an array of 3 vertices
	triangle BasicVertex input[3],

	// Second parameter is used for output - we will create a triangle strip (stream) to replace the single input triangle
	inout TriangleStream<LightingPixelShaderInput> outStrip
)
{
	// Calculate the face normal - get two edges of the triangle and do a cross product to get it
	float3 faceEdgeA = input[1].position - input[0].position;
	float3 faceEdgeB = input[2].position - input[0].position;
	float3 faceNormal = normalize(cross(faceEdgeA, faceEdgeB));
	float3 explodeVector = faceNormal * gExplodeAmount;


	// Outputing a triangle strip, use this variable to build the output vertices one at a time
	LightingPixelShaderInput output; // The data we output here is eventually sent to the pixel shader (after the triangle is rasterized into pixels)

	// Output a new triangle (to replace the input triangle) - use a for loop to iterate through the 3 vertices
	// Ouput each vertex after exploding it outwards in the face normal direction
	for (int i = 0; i < 3; ++i)
	{
		// Get exploded position, this is the new world position of the current vertex
		float3 explodePosition = input[i].position + explodeVector;
		output.worldPosition = explodePosition;

		// The vertex shader didn't transform the vertices into 2D, since this geometry shader needed to work on the vertices while still
		// in world space. So now the geometry shader needs to perform that transform before outputing the vertices to the pixel shader
		output.projectedPosition = mul(gViewProjectionMatrix, float4(explodePosition, 1));

		// Pass on the world normal and texture coordinate to be used by pixel shader
		output.worldNormal = input[i].normal;
		output.uv = input[i].uv;

		// Add this new vertex to the output stream
		outStrip.Append(output);
	}

	// The output stream is a collection of triangle strips. At the end of each strip, we must call RestartStrip
	// This example is a trivial case where there is just 1 strip with just 1 triangle in it, but we still call this at the end
	outStrip.RestartStrip();
	
    for (int i = 0; i < 3; ++i)
    {
		
        output.worldPosition = input[i].position;

		// The vertex shader didn't transform the vertices into 2D, since this geometry shader needed to work on the vertices while still
		// in world space. So now the geometry shader needs to perform that transform before outputing the vertices to the pixel shader
        output.projectedPosition = mul(gViewProjectionMatrix, float4(input[i].position, 1));

		// Pass on the world normal and texture coordinate to be used by pixel shader
        output.worldNormal = input[i].normal;
        output.uv = input[i].uv;

		// Add this new vertex to the output stream
        outStrip.Append(output);
    }
	
    outStrip.RestartStrip();
}
