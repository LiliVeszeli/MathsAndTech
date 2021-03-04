//**************************************************************************************
// Tessellation - Domain Shader
//**************************************************************************************

// The GPU uses the tessellation factors you calculate during the hull shader stage to automatically tessellate a generic unit square.
// It is the domain shader's task to convert this generic tessellated geometry into the location/shape required by the patch we are currently
// rendering. To do this, the domain shader gets the patch control points output from the hull shader and the output of the patch control function.
//
// The domain shader receives the generic tessellation one vertex at a time. When working with quad patches these vertex coordinates are in a
// 0->1 unit square and look a bit like texture coordinates. However, when working with triangle patches the generic tessellation vertices use
// "Barycentric coordinates". Barycentric coordinates are a good way to specify a position within an arbitrary triangle.
// If the triangle points are P0,P1 and P2, then the Barycentric coordinates a,b,c define the point:
//   P = a*P0 + b*P1 + c*P2
// In other words add together a triangle's three corner points (or normals or whatever) with the Barycentric coordinates as weightings to find
// the point within the triangle being referred to. (Note: a + b + c will always be 1).
//
// Effectively Barycentric coordinates define a coordinate system with one triangle point as the origin and the two radiating edges as axes.
// Barycentric coordinates occasionally crop up in graphics (or physics) when defining points in triangles, although new, they are not at all complex
// to use. See the Van Verth maths text for further information

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// The domain shader needs the height map to do the displacment mapping - pushing each tessellated vertex outwards based on
// the height map to give high polygon bumpy geometry
Texture2D NormalHeightMap : register(t0);
SamplerState TexSampler   : register(s0);


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

[domain("tri")]
NormalMappingPixelShaderInput main
(
	PatchConstantOutput input,                 // The output from the patch constant function, unused in this example, but available if needed
	float3 genericVertex : SV_DomainLocation,  // The generic tessellated vertex to work on, defined in barycentric coordinates
    const OutputPatch<HullShaderOutput, 3> trianglePatch  // The control points defining the patch we are rendering (output from hull shader)
	                                                      // Domain shader's job is to transform the generic vertex into this patch - easy here since our patches just simple triangles
)
{
    NormalMappingPixelShaderInput output;

    // Interpolate points of our patch (triangle) with the given barycentric coordinates of the generic vertex
    output.worldPosition = genericVertex.x * trianglePatch[0].worldPosition + 
                           genericVertex.y * trianglePatch[1].worldPosition + 
                           genericVertex.z * trianglePatch[2].worldPosition;
    
    // Interpolate world space normal & tangent in the same way - renormalize afterwards
    output.worldNormal = genericVertex.x * trianglePatch[0].worldNormal + 
                         genericVertex.y * trianglePatch[1].worldNormal + 
                         genericVertex.z * trianglePatch[2].worldNormal;
    output.worldNormal = normalize( output.worldNormal );

	output.worldTangent = //**** MISSING calculate output worldTangent based on the generic tessellation and the triangle patch tangents (the original tangents)
    output.worldTangent = normalize( output.worldTangent );
    
    // Same again for UVs
    output.uv = genericVertex.x * trianglePatch[0].uv + 
                genericVertex.y * trianglePatch[1].uv + 
                genericVertex.z * trianglePatch[2].uv;

    // Sample normal / height map. Domain shaders cannot automatically use mip-maps (only the pixel shader can do this automatically),
	// so we have to use the "SampleLevel" statement (instead of "Sample") and explicitly say which mip-map to use.
	// Just choosing the highest detail mip-map here (the final 0), but you can calculate a value (from 0 to number mip-maps - 1) based on
	// distance, which will improve performance. See the DirectX detail tessellation sample for an example of setting an explicit mip-map
    float displacementHeight = NormalHeightMap.SampleLevel(TexSampler, output.uv, 0).w; // Sample height only (the normal is in xyz and the height is in w)
    
    // Displace the output world position along world normal by height from the height map. Also multiply the displacement by gDisplacementScale to give us
	// control over the overall depth of the effect
    //**** MISSING: Write this critical line for displacement mapping yourself...
    
    // Transform world position with view and projection matrices
    float4 viewPosition      = mul(gViewMatrix,       float4(output.worldPosition, 1.0f));
    output.projectedPosition = mul(gProjectionMatrix, viewPosition);

    return output;
}

