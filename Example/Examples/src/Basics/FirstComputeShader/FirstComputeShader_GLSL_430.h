/*********************************************************\
 * Copyright (c) 2012-2018 The Unrimp Team
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
#ifdef RENDERER_OPENGL
if (renderer->getNameId() == Renderer::NameId::OPENGL)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(#version 430 core	// OpenGL 4.3

// Attribute input/output
in  vec2 Position;	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
out gl_PerVertex
{
	vec4 gl_Position;
};
out vec2 TexCoord;	// Normalized texture coordinate as output

// Programs
void main()
{
	// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	gl_Position = vec4(Position, 0.5, 1.0);
	TexCoord = Position.xy;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 430 core	// OpenGL 4.3

// Attribute input/output
in  vec2 TexCoord;		// Normalized texture coordinate as input
out vec4 OutputColor;	// Output variable for fragment color

// Uniforms
uniform sampler2D AlbedoMap;

// Programs
void main()
{
	// Fetch the texel at the given texture coordinate and return its color
	OutputColor = texture2D(AlbedoMap, TexCoord);
}
)";


//[-------------------------------------------------------]
//[ Compute shader source code                            ]
//[-------------------------------------------------------]
computeShaderSourceCode = R"(#version 430 core	// OpenGL 4.3
// Same layout as "Renderer::DrawInstancedArguments"
struct DrawInstancedArguments
{
	uint vertexCountPerInstance;
	uint instanceCount;
	uint startVertexLocation;
	uint startInstanceLocation;
};

// Uniforms
layout(binding = 0)					  uniform sampler2D InputTextureMap;
layout(binding = 1, rgba8)	writeonly uniform image2D   OutputTextureMap;
layout(binding = 2, std430) writeonly		  buffer    OutputIndirectBuffer
{
	DrawInstancedArguments drawInstancedArguments;
};

// Programs
layout (local_size_x = 16, local_size_y = 16) in;
void main()
{
	// Fetch input texel
	vec4 color = texelFetch(InputTextureMap, ivec2(gl_GlobalInvocationID.xy), 0);

	// Modify color
	color.g *= 1.0f - (float(gl_GlobalInvocationID.x) / 16.0f);
	color.g *= 1.0f - (float(gl_GlobalInvocationID.y) / 16.0f);

	// Output texel
	imageStore(OutputTextureMap, ivec2(gl_GlobalInvocationID.xy), color);

	// Output draw call
	if (0 == gl_GlobalInvocationID.x && 0 == gl_GlobalInvocationID.y && 0 == gl_GlobalInvocationID.z)
	{
		drawInstancedArguments.vertexCountPerInstance = 3;
		drawInstancedArguments.instanceCount		  = 1;
		drawInstancedArguments.startVertexLocation    = 0;
		drawInstancedArguments.startInstanceLocation  = 0;
	}
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
