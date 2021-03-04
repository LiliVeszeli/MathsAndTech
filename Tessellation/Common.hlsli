//--------------------------------------------------------------------------------------
// Common include file for all shaders
//--------------------------------------------------------------------------------------
// Using include files to define the type of data passed between the shaders


//--------------------------------------------------------------------------------------
// Shader input / output
//--------------------------------------------------------------------------------------

// The structure below describes the model vertex data provided to the vertex shader for ordinary non-skinned models
struct BasicVertex
{
    float3 position : position;
    float3 normal   : normal;
    float2 uv       : uv;
};


// This structure describes what data the lighting pixel shader receives from the vertex shader.
// The projected position is a required output from all vertex shaders - where the vertex is on the screen
// The world position and normal at the vertex are sent to the pixel shader for the lighting equations.
// The texture coordinates (uv) are passed from vertex shader to pixel shader unchanged to allow textures to be sampled
struct LightingPixelShaderInput
{
    float4 projectedPosition : SV_Position; // This is the position of the pixel to render, this is a required input
                                            // to the pixel shader and so it uses the special semantic "SV_Position"
                                            // because the shader needs to identify this important information
    
    float3 worldPosition : worldPosition;   // The world position and normal of each vertex is passed to the pixel...
    float3 worldNormal   : worldNormal;     //...shader to calculate per-pixel lighting. These will be interpolated
                                            // automatically by the GPU (rasterizer stage) so each pixel will know
                                            // its position and normal in the world - required for lighting equations
    
    float2 uv : uv; // UVs are texture coordinates. The artist specifies for every vertex which point on the texture is "pinned" to that vertex.
};


// This structure is similar to the one above but for the light models, which aren't themselves lit
struct SimplePixelShaderInput
{
    float4 projectedPosition : SV_Position;
    float2 uv                : uv;
};


//**************************************************************************************
// Tessellation shader input / outputs
//**************************************************************************************

// The structure below describes the model vertex data provided to the tessellation vertex shader
// This is the same data that a normal mapped or parallaxed map model would use, so tessellation adds no burden on the modelling/data side
struct NormalMappingVertex
{
    float3 position : position;
    float3 normal   : normal;
    float3 tangent  : tangent;
    float2 uv       : uv;
};

// This structure describes what data the first tessellation stage (hull shader) receives from the vertex shader
// The tessellation occurs in world space, the the projection to 2D hasn't happened yet as it would in typical usage
struct HullShaderInput
{
    float3 worldPosition : worldPosition;
    float3 worldNormal   : worldNormal;
    float3 worldTangent  : worldTangent;
    float2 uv            : uv;
};

// The data output by the hull shader. This is the input to the next stage, the domain shader. It is also provided to the
// patch constant function (see tessellation hull shader file for details). The hull shader is only used for specialist
// purposes - in this example it copies most of the data directly from the vertex shader output with no change, so this data
// will be mostly the same as the structure above. However, it does provide a tessellation factor to the patch constant
// function to avoid cracks at the seams (see code in hull shader file)
struct HullShaderOutput
{
    float3 worldPosition : worldPosition;
    float3 worldNormal   : worldNormal;
    float3 worldTangent  : worldTangent;
    float2 uv            : uv;
	float  oppositeEdgeFactor : oppositeEdgeFactor;
};

// This is the data that the "patch constant function" outputs. Note the SV_ semantics - these are required outputs
// This function is called automatically in conjunction with the hull shader to decide the amount of tessellation each patch needs
struct PatchConstantOutput
{
    float edgeFactor[3] : SV_TessFactor; // Patches are triangles in this example hence the 3
    float insideFactor  : SV_InsideTessFactor;
};


// The tessellation code uses a fairly typical normal mapping pixel shader - the tessellation has already been done by this point
// So the final output from the tessallation stages (domain shader) is the same data we expect for normal mapping
struct NormalMappingPixelShaderInput
{
    float4 projectedPosition : SV_Position;
    float3 worldPosition     : worldPosition;
    float3 worldNormal       : worldNormal;
    float3 worldTangent      : worldTangent;
    float2 uv                : uv;
};



//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

// These structures are "constant buffers" - a way of passing variables over from C++ to the GPU
// They are called constants but that only means they are constant for the duration of a single GPU draw call.
// These "constants" correspond to variables in C++ that we will change per-model, or per-frame etc.

// In this exercise the matrices used to position the camera are updated from C++ to GPU every frame along with lighting information
// These variables must match exactly the gPerFrameConstants structure in Scene.cpp
cbuffer PerFrameConstants : register(b0) // The b0 gives this constant buffer the number 0 - used in the C++ code
{
	float4x4 gCameraMatrix;         // World matrix for the camera (inverse of the ViewMatrix below)
	float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float4x4 gViewProjectionMatrix; // The above two matrices multiplied together to combine their effects

    float3   gLight1Position; // 3 floats: x, y z
    float    gViewportWidth;  // Using viewport width and height as padding - see this structure in earlier labs to read about padding here
    float3   gLight1Colour;
    float    gViewportHeight;

    float3   gLight2Position;
    float    padding1;
    float3   gLight2Colour;
    float    padding2;

    float3   gAmbientColour;
    float    gSpecularPower;

    float3   gCameraPosition;
    float    padding3;
}
// Note constant buffers are not structs: we don't use the name of the constant buffer, these are really just a collection of global variables (hence the 'g')



static const int MAX_BONES = 64;

// If we have multiple models then we need to update the world matrix from C++ to GPU multiple times per frame because we
// only have one world matrix here. Because this data is updated more frequently it is kept in a different buffer for better performance.
// We also keep other data that changes per-model here
// These variables must match exactly the gPerModelConstants structure in Scene.cpp
cbuffer PerModelConstants : register(b1) // The b1 gives this constant buffer the number 1 - used in the C++ code
{
    float4x4 gWorldMatrix;

    float3   gObjectColour;  // Useed for tinting light models
	float    padding4;

	//**** Displacement mapping settings
	float    gMinTessellation;           // Minimum tessellation factor, i.e. maximum detail level (used in hull shader)
	float    gMaxTessellation;           // --"-- maximum
	float    gMinTessellationDistance;   // Distance at which *maximum* tessellation will be used (used in hull shader)
	float    gMaxTessellationDistance;   // --"-- minimum
		     
	float    gDisplacementScale; // Depth of displacement mapping - displacement in world units for the maximum height in the height map
    float3   padding5;
	//************

	float4x4 gBoneMatrices[MAX_BONES];
}
