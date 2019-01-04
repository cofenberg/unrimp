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
#if defined(RENDERER_DIRECT3D10) || defined(RENDERER_DIRECT3D11) || defined(RENDERER_DIRECT3D12)
if (renderer->getNameId() == Renderer::NameId::DIRECT3D10 ||renderer->getNameId() == Renderer::NameId::DIRECT3D11 || renderer->getNameId() == Renderer::NameId::DIRECT3D12)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(
// Attribute input/output
struct VS_INPUT
{
	float2 Position   : POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
	uint   InstanceID : SV_INSTANCEID;
};
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
};

// Programs
VS_OUTPUT main(VS_INPUT input)
{
	// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	VS_OUTPUT output;
	output.Position = float4(input.Position.x, input.Position.y - float(input.InstanceID), 0.5f, 1.0f);
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
	// Return blue
	return float4(0.0f, 0.0f, 1.0f, 1.0f);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif