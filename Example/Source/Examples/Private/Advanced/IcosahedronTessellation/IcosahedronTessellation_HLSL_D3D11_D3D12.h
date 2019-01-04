/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


// Shaders are from: http://prideout.net/blog/?p=48 (Philip Rideout, "The Little Grasshopper - Graphics Programming Tips")
// -> Ported from GLSL to HLSL


//[-------------------------------------------------------]
//[ Shader start                                          ]
//[-------------------------------------------------------]
#if defined(RENDERER_DIRECT3D11) || defined(RENDERER_DIRECT3D12)
if (renderer->getNameId() == Renderer::NameId::DIRECT3D11 || renderer->getNameId() == Renderer::NameId::DIRECT3D12)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per control point of the patch
vertexShaderSourceCode = R"(
// Attribute input/output
struct VS_OUTPUT
{
	float3 Position : POSITION;	// Object space control point position of the patch as output
};

// Programs
VS_OUTPUT main(float3 Position : POSITION)	// Object space control point position of the patch we received from the input assembly (IA) as input
{
	// Pass through the object space control point position of the patch
	VS_OUTPUT output;
	output.Position = Position;
	return output;
}
)";


//[-------------------------------------------------------]
//[ Tessellation control shader source code               ]
//[-------------------------------------------------------]
// Under Direct3D 11, the tessellation control shader invocation is slit into per patch and per patch control point
// "hull shader" in Direct3D terminology
tessellationControlShaderSourceCode = R"(
// Attribute input/output
struct VS_OUTPUT
{
	float3 Position : POSITION;	// Object space control point position of the patch we received from the vertex shader (VS) as input
};
struct HS_CONSTANT_DATA_OUTPUT
{
	float TessLevelInner[1] : SV_INSIDETESSFACTOR;	// Inner tessellation level
	float TessLevelOuter[3] : SV_TESSFACTOR;		// Outer tessellation level
};
struct HS_OUTPUT
{
	float3 Position : POSITION;	// Object space control point position of the patch as output
};

// Uniforms
cbuffer UniformBlockDynamicTcs : register(b0)
{
	float TessellationLevelOuter;	// Outer tessellation level
	float TessellationLevelInner;	// Inner tessellation level
}

// Program invocation per patch
HS_CONSTANT_DATA_OUTPUT ConstantHS(InputPatch<VS_OUTPUT, 3> input, uint PatchID : SV_PRIMITIVEID)
{
	// Inform the tessellator about the desired tessellation level
	HS_CONSTANT_DATA_OUTPUT output;
	output.TessLevelOuter[0] = TessellationLevelOuter;
	output.TessLevelOuter[1] = TessellationLevelOuter;
	output.TessLevelOuter[2] = TessellationLevelOuter;
	output.TessLevelInner[0] = TessellationLevelInner;
	return output;
}

// Program invocation per patch control point
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> input, uint InvocationID : SV_OutputControlPointID)
{
	// Pass through the object space control point position of the patch
	HS_OUTPUT output;
	output.Position = input[InvocationID].Position;
	return output;
}
)";


//[-------------------------------------------------------]
//[ Tessellation evaluation shader source code            ]
//[-------------------------------------------------------]
// One tessellation evaluation shader invocation per point from tessellator
// "domain shader" in Direct3D terminology
tessellationEvaluationShaderSourceCode = R"(
// Attribute input/output
struct HS_CONSTANT_DATA_OUTPUT
{
	float TessLevelInner[1] : SV_INSIDETESSFACTOR;	// Inner tessellation level
	float TessLevelOuter[3] : SV_TESSFACTOR;		// Outer tessellation level
};
struct HS_OUTPUT
{
	float3 Position : POSITION;	// Object space control point position of the patch we received from the tessellation control shader (TCS) as input
};
struct DS_OUTPUT
{
	float3 Position			 : POSITION;	// Interpolated object space vertex position inside the patch as output
	float3 PatchDistance	 : TEXCOORD0;	// The barycentric coordinate inside the patch we received from the tessellator as output
	float4 ClipSpacePosition : TEXCOORD1;	// Clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1) as output
};

// Uniforms
cbuffer UniformBlockStaticTes : register(b0)
{
	float4x4 ObjectSpaceToClipSpaceMatrix;	// Object space to clip space matrix
}

// Programs
[domain("tri")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT inputTess, const OutputPatch<HS_OUTPUT, 3> input, float3 TessCoord : SV_DOMAINLOCATION)
{
	DS_OUTPUT output;

	// The barycentric coordinate "TessCoord" we received from the tessellator defines a location
	// inside a triangle as a combination of the weight of the three control points of the patch

	// Calculate the vertex object space position inside the patch by using the barycentric coordinate
	// we received from the tessellator and the three object space control points of the patch
	float3 p0 = TessCoord.x * input[0].Position;
	float3 p1 = TessCoord.y * input[1].Position;
	float3 p2 = TessCoord.z * input[2].Position;
	output.Position = normalize(p0 + p1 + p2);

	// Pass through the barycentric coordinate inside the patch
	output.PatchDistance = TessCoord;

	// Calculate the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	output.ClipSpacePosition = mul(ObjectSpaceToClipSpaceMatrix, float4(output.Position, 1.0));

	// Done
	return output;
}
)";


//[-------------------------------------------------------]
//[ Geometry shader source code                           ]
//[-------------------------------------------------------]
// One geometry shader invocation per primitive
geometryShaderSourceCode = R"(
// Attribute input/output
struct DS_OUTPUT
{
	float3 Position			 : POSITION;	// Interpolated object space vertex position inside the patch we received from the tessellation evaluation shader (TES) as input
	float3 PatchDistance	 : TEXCOORD0;	// The barycentric coordinate inside the patch from the tessellator we received from the tessellation evaluation shader (TES) as input
	float4 ClipSpacePosition : TEXCOORD1;	// Clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1) we received from the tessellation evaluation shader (TES) as input
};
struct GS_OUTPUT
{
	float4 Position      : SV_POSITION;	// The clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1), as output
	float3 FacetNormal   : TEXCOORD0;	// Normalized normal of the primitive as output
	float3 PatchDistance : TEXCOORD1;	// The barycentric coordinate inside the patch from the tessellator as output
	float3 TriDistance   : TEXCOORD2;	// Triangle vertex as output
};

// Uniforms
cbuffer UniformBlockStaticGs : register(b0)
{
	// TODO(co) float3x3
	float4x4 NormalMatrix;	// Object space to clip space rotation matrix
}

[maxvertexcount(3)]
void main(triangle DS_OUTPUT input[3], inout TriangleStream<GS_OUTPUT> OutputStream)
{
	GS_OUTPUT output;

	float3 A = input[2].Position - input[0].Position;
	float3 B = input[1].Position - input[0].Position;
	// TODO(co) float3x3
	output.FacetNormal = mul(NormalMatrix, float4(normalize(cross(A, B)), 1.0f)).xyz;

	// Emit vertex 0
	output.PatchDistance = input[0].PatchDistance;
	output.TriDistance = float3(1.0f, 0.0f, 0.0f);
	output.Position = input[0].ClipSpacePosition;
	OutputStream.Append(output);

	// Emit vertex 1
	output.PatchDistance = input[1].PatchDistance;
	output.TriDistance = float3(0.0f, 1.0f, 0.0f);
	output.Position = input[1].ClipSpacePosition;
	OutputStream.Append(output);

	// Emit vertex 2
	output.PatchDistance = input[2].PatchDistance;
	output.TriDistance = float3(0.0f, 0.0f, 1.0f);
	output.Position = input[2].ClipSpacePosition;
	OutputStream.Append(output);

	// Done
	OutputStream.RestartStrip();
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
// "pixel shader" in Direct3D terminology
fragmentShaderSourceCode = R"(
// Attribute input/output
struct GS_OUTPUT
{
	float4 Position      : SV_POSITION;	// The clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1), we received from the geometry shader (GS) as input
	float3 FacetNormal   : TEXCOORD0;	// Normalized normal of the primitive we received from the geometry shader (GS) as input
	float3 PatchDistance : TEXCOORD1;	// The barycentric coordinate inside the patch from the tessellator we received from the geometry shader (GS) as input
	float3 TriDistance   : TEXCOORD2;	// Triangle vertex we received from the geometry shader (GS) as input
};

// Uniforms
cbuffer UniformBlockStaticFs : register(b0)
{
	// TODO(co) float3
	float4 LightPosition;
	float4 DiffuseMaterial;
	float4 AmbientMaterial;
}

// Programs
float Amplify(float d, float scale, float offset)
{
	d = scale * d + offset;
	d = clamp(d, 0.0f, 1.0f);
	d = 1.0f - exp2(-2.0f * d * d);
	return d;
}

float4 main(GS_OUTPUT input) : SV_TARGET
{
	// Simple lighting
	float3 N = normalize(input.FacetNormal);
	float3 L = LightPosition.xyz;// TODO(co) float3
	float df = abs(dot(N, L));
	float3 color = AmbientMaterial.rgb + df * DiffuseMaterial.rgb;// TODO(co) float3

	// Add wireframe via fragment color manipulation
	// -> Thick (60) black lines for the patch edges
	// -> Thin (40) black lines for the edges of the generated triangles inside the patch
	// -> Determine how far the current fragment is from the nearest edge of the patch and
	//    local triangle by use the given interpolated positions inside the patch and local triangle
	float d1 = min(min(input.TriDistance.x, input.TriDistance.y), input.TriDistance.z);
	float d2 = min(min(input.PatchDistance.x, input.PatchDistance.y), input.PatchDistance.z);
	color = Amplify(d1, 40.0f, -0.5f) * Amplify(d2, 60.0f, -0.5f) * color;

	// Return the calculated color
	return float4(color, 1.0f);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
