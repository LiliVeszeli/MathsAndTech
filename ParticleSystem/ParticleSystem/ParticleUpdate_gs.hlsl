//--------------------------------------------------------------------------------------
// GPU particle update geometry shader
//--------------------------------------------------------------------------------------
// Geometry shader that updates the positions of a particles (passed as a point), then
// uses the "stream out" stage to rewrite it into the vertex buffer.
// Updating particles on the GPU means we don't have to copy data between GPU and CPU
// memory each frame, allowing for very large numbers of particles.
// Does not perform any rendering.

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

[maxvertexcount(1)]  // This shader reads one particle point, updates it and outputs it again
void main
(
	point ParticleData               input[1], // One particle in, as a point
	inout PointStream<ParticleData>  output    // Must output a point stream, will only contain the one updated point
)
{
	const float3 gravity = { 0, -9.8f, 0 };
	
    
    

	// Fairly general purpose particle update code
    input[0].life -= gFrameTime;
    if (input[0].life <= 0)
    {
        input[0].life = 5.0f;
        input[0].position = float3(0, 60, 0);

    }
  
    if (input[0].position.y <= 0)
    {
        input[0].velocity.y = -input[0].velocity.y;

    }
    input[0].position += input[0].velocity * gFrameTime;
	input[0].velocity += gravity * gFrameTime;
	output.Append(input[0]);
}