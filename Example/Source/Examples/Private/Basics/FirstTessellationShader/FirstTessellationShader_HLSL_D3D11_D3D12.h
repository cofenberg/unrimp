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
	float2 Position : POSITION;	// Clip space control point position of the patch as input, left/bottom is (-1,-1) and right/top is (1,1)
};

// Programs
VS_OUTPUT main(float2 Position : POSITION)
{
	// Pass through the clip space control point position of the patch, left/bottom is (-1,-1) and right/top is (1,1)
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
	float2 Position : POSITION;	// Clip space control point position of the patch we received from the vertex shader (VS) as input
};
struct HS_CONSTANT_DATA_OUTPUT
{
	float TessLevelOuter[3] : SV_TESSFACTOR;		// Outer tessellation level
	float TessLevelInner[1] : SV_INSIDETESSFACTOR;	// Inner tessellation level
};
struct HS_OUTPUT
{
	float2 Position : POSITION;	// Clip space control point position of the patch as output
};

// Program invocation per patch
HS_CONSTANT_DATA_OUTPUT ConstantHS(InputPatch<VS_OUTPUT, 3> input)
{
	// Inform the tessellator about the desired tessellation level
	HS_CONSTANT_DATA_OUTPUT output;
	output.TessLevelOuter[0] = 1.0f;
	output.TessLevelOuter[1] = 2.0f;
	output.TessLevelOuter[2] = 3.0f;
	output.TessLevelInner[0] = 4.0f;
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
	// Pass through the clip space control point position of the patch
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
	float TessLevelOuter[3] : SV_TESSFACTOR;		// Outer tessellation level
	float TessLevelInner[1] : SV_INSIDETESSFACTOR;	// Inner tessellation level
};
struct HS_OUTPUT
{
	float2 Position : POSITION;	// Clip space control point position of the patch we received from the tessellation control shader (TCS) as input
};
struct DS_OUTPUT
{
	float4 Position : SV_POSITION;	// Interpolated clip space control point position inside the patch as output
};

// Programs
[domain("tri")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT inputTess, float3 TessCoord : SV_DOMAINLOCATION, const OutputPatch<HS_OUTPUT, 3> input)
{
	DS_OUTPUT output;

	// The barycentric coordinate "TessCoord" we received from the tessellator defines a location
	// inside a triangle as a combination of the weight of the three control points of the patch

	// Calculate the vertex clip space position inside the patch by using the barycentric coordinate
	// we received from the tessellator and the three clip space control points of the patch
	float2 p0 = TessCoord.x * input[0].Position;
	float2 p1 = TessCoord.y * input[1].Position;
	float2 p2 = TessCoord.z * input[2].Position;
	output.Position = float4(p0 + p1 + p2, 0.5f, 1.0f);

	// Done
	return output;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
// "pixel shader" in Direct3D terminology
fragmentShaderSourceCode = R"(
// Programs
float4 main(float4 Position : SV_POSITION) : SV_TARGET
{
	// Return white
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
