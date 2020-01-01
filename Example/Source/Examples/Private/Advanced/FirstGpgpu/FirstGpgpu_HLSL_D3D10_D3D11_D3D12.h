/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#if defined(RHI_DIRECT3D10) || defined(RHI_DIRECT3D11) || defined(RHI_DIRECT3D12)
if (mRhi->getNameId() == Rhi::NameId::DIRECT3D10 || mRhi->getNameId() == Rhi::NameId::DIRECT3D11 || mRhi->getNameId() == Rhi::NameId::DIRECT3D12)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(
// Attribute input/output
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
	float2 TexCoord : TEXCOORD0;	// Normalized texture coordinate as output
};

// Programs
VS_OUTPUT main(float2 Position : POSITION)	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
{
	VS_OUTPUT output;

	// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	output.Position = float4(Position, 0.5f, 1.0f);

	// Calculate the texture coordinate by mapping the clip space coordinate to a texture space coordinate
	// -> Unlike OpenGL or OpenGL ES 3, in Direct3D 9 & 10 & 11 the texture origin is left/top which does not map well to clip space coordinates
	// -> We have to flip the y-axis to map the coordinate system to the Direct3D 9 & 10 & 11 texture coordinate system
	// -> In this example we do this within the fragment shader so have identical wobble across the different graphics APIs
	// -> (-1,-1) -> (0,0)
	// -> (1,1) -> (1,1)
	output.TexCoord = Position.xy * 0.5f + 0.5f;

	// Done
	return output;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code - Content generation      ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
// "pixel shader" in Direct3D terminology
fragmentShaderSourceCode_ContentGeneration = R"(
// Programs
float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Return the color green
	return float4(0.0f, 1.0f, 0.0f, 1.0f);
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code - Content processing      ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
// "pixel shader" in Direct3D terminology
fragmentShaderSourceCode_ContentProcessing = R"(
// Uniforms
SamplerState SamplerLinear : register(s0);
Texture2D ContentMap : register(t0);

// Programs
float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Fetch the texel at the given texture coordinate and return its color
	// -> Apply a simple wobble to the texture coordinate so we can see that content processing is up and running
	// -> Unlike OpenGL or OpenGL ES 3, in Direct3D 9 & 10 & 11 the texture origin is left/top which does not map well to clip space coordinates
	// -> We have to flip the y-axis to map the coordinate system to the Direct3D 9 & 10 & 11 texture coordinate system
	// -> (-1,-1) -> (0,1)
	// -> (1,1) -> (1,0)
	return ContentMap.Sample(SamplerLinear, float2(TexCoord.x + sin(TexCoord.x * 100.0f) * 0.01f, 1.0f - TexCoord.y - cos(TexCoord.y * 100.0f) * 0.01f));
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
