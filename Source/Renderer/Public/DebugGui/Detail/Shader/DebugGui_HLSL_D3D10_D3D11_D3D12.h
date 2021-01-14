/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
if (rhi.getNameId() == Rhi::NameId::DIRECT3D10 || rhi.getNameId() == Rhi::NameId::DIRECT3D11 || rhi.getNameId() == Rhi::NameId::DIRECT3D12)
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
	float4 Color    : COLOR0;		// sRGB vertex color
};

// Uniforms
cbuffer UniformBlockDynamicVs : register(b0)
{
	float4x4 ObjectSpaceToClipSpaceMatrix;
}

// Programs
VS_OUTPUT main(float4 PositionTexCoord : POSITION,	// xy = clip space vertex position as input with left/bottom is (-1,-1) and right/top is (1,1), zw = normalized 32 bit texture coordinate as input
			   float4 Color            : COLOR0)	// sRGB vertex color
{
	VS_OUTPUT output;

	// Calculate the clip space vertex position, lower/left is (-1,-1) and upper/right is (1,1)
	output.Position = mul(ObjectSpaceToClipSpaceMatrix, float4(PositionTexCoord.xy, 0.5f, 1.0f));

	// Pass through the vertex texture coordinate
	output.TexCoord = PositionTexCoord.zw;

	// Pass through the vertex color
	output.Color = Color;

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
// Uniforms
SamplerState SamplerLinear : register(s0);
Texture2D GlyphMap : register(t0);	// Glyph atlas texture map

// Programs
// -> Input vertex color is in sRGB, so is the fragment color output
float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0, float4 Color : COLOR0) : SV_TARGET
{
	// Fetch the texel at the given texture coordinate and return its color
	return Color * float4(1.0f, 1.0f, 1.0f, GlyphMap.Sample(SamplerLinear, TexCoord).r);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
