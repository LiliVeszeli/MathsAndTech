//--------------------------------------------------------------------------------------
// Tessellation Vertex Shader
//--------------------------------------------------------------------------------------
// Vertex shader for tessellation is just the same as the usual vertex shader except it only transforms the model geometry into world space,
// hull shader works in world space, and the domain shader handles the transformation into screen coordinates (with the view and projection matrix)

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

HullShaderInput main(NormalMappingVertex input)
{
	HullShaderInput output;

	// Transform the input model vertex position into world space
	float4 modelPosition = float4(input.position, 1.0f);
	output.worldPosition = mul(gWorldMatrix, modelPosition).xyz;

	// Transform the vertex normal and tangent from model space into world space
	float4 modelNormal  = float4(input.normal, 0.0f);
	float4 modelTangent = float4(input.tangent, 0.0f);
	output.worldNormal  = mul(gWorldMatrix, modelNormal).xyz;
	output.worldTangent = mul(gWorldMatrix, modelTangent).xyz;

	// Pass texture coordinates (UVs) on unchanged
	output.uv = input.uv;

	return output;
}