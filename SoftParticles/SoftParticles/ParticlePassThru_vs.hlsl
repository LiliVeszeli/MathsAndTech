//--------------------------------------------------------------------------------------
// GPU particle rendering geometry shader
//--------------------------------------------------------------------------------------
// Vertex shader that passes on particle data to the geometry shader without change
// The geometry shaders do all the update rendering work for this particle system

#include "Common.hlsli"


//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function
void main(in ParticleData input, out ParticleData output)
{
	output = input;
}
