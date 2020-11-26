//--------------------------------------------------------------------------------------
// GPU particle rendering geometry shader
//--------------------------------------------------------------------------------------
// Geometry shader that renders a particle.The particle is passed in as a points and
// is converted to a camera facing quad (two triangles)

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

[maxvertexcount(4)]  // Each particle point is converted to a quad (two triangles)
void main
(
	point ParticleData                             input[1], // One particle in, as a point
	inout TriangleStream<ParticlePixelShaderInput> output    // Triangle stream output, a quad containing two triangles
)
{
	// Camera-space offsets for the four corners of the quad from the particle centre
	// Will be scaled depending on distance of the particle
	const float3 cameraSpaceCorners[4] =
	{
        float3(-0.5,  0.5, 0),
        float3( 0.5,  0.5, 0),
        float3(-0.5, -0.5, 0),
        float3( 0.5, -0.5, 0),
	};

	// Texture coordinates for the four corners of the generated quad
	const float2 uvCorners[4] =
	{
		float2(0,0),
		float2(1,0),
		float2(0,1),
		float2(1,1),
	};

	ParticlePixelShaderInput outVert; // Used to build output vertices

	// Output the four corner vertices of a quad centred on the particle position
    float s = sin(input[0].rotation);
    float c = cos(input[0].rotation);
    float3x3 rotation = float3x3(c, -s,  0,
                                 s,  c,  0,
                                 0,  0,  1);
	for (int i = 0; i < 4; ++i)
	{
		// Use the corners defined above and the inverse camera matrix to calculate each world space corner of the quad
		float3 corner = mul(rotation, cameraSpaceCorners[i] * input[0].scale);
		float3 worldPosition = input[0].position + mul((float3x3)gCameraMatrix, corner);

		// Transform to 2D position and output along with an appropriate UV
		outVert.projectedPosition = mul(gViewProjectionMatrix, float4(worldPosition, 1.0f));
		outVert.uv = uvCorners[i];
		
        // Copy other particle data
        outVert.alpha = input[0].alpha;
        outVert.scale = input[0].scale;

        output.Append(outVert);
	}
	output.RestartStrip();
}