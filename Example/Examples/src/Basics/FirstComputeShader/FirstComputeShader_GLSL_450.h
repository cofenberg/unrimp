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
if (renderer->getNameId() == Renderer::NameId::VULKAN)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(#version 450 core	// OpenGL 4.5
struct Vertex
{
	vec2 position;
	vec2 padding;
};

// Attribute input/output
layout(location = 0) in  vec2 Position;	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
layout(location = 0) out gl_PerVertex
{
	vec4 gl_Position;
};
layout(location = 1) out vec2 TexCoord;	// Normalized texture coordinate as output

// Uniforms
layout(set = 0, binding = 1) uniform samplerBuffer InputTextureBuffer;
layout(std430, set = 0, binding = 2) readonly buffer InputStructuredBuffer	// TODO(co) Triggers "Fix NonWritable check when vertexPipelineStoresAndAtomics not enabled #2526" - https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/2526
{
	Vertex inputStructuredBufferVertex[];
};

// Programs
void main()
{
	// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	gl_Position = vec4(Position + texelFetch(InputTextureBuffer, gl_VertexIndex).xy + inputStructuredBufferVertex[gl_VertexIndex].position, 0.5f, 1.0f);
	TexCoord = Position.xy;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 450 core	// OpenGL 4.5

// Attribute input/output
layout(location = 1) in  vec2 TexCoord;		// Normalized texture coordinate as input
layout(location = 0) out vec4 OutputColor;	// Output variable for fragment color

// Uniforms
layout(std140, set = 0, binding = 0) uniform UniformBuffer
{
	vec4 inputColorUniform;
};
layout(set = 0, binding = 3) uniform sampler2D AlbedoMap;

// Programs
void main()
{
	// Fetch the texel at the given texture coordinate and return its color
	OutputColor = texture(AlbedoMap, TexCoord) * inputColorUniform;
}
)";


//[-------------------------------------------------------]
//[ Compute shader source code                            ]
//[-------------------------------------------------------]
computeShaderSourceCode1 = R"(#version 450 core	// OpenGL 4.5
struct Vertex
{
	vec2 position;
};

// Input
layout(set = 0, binding = 0) uniform sampler2D InputTexture2D;
layout(std430, set = 0, binding = 1) readonly buffer InputIndexBuffer
{
	uint inputIndices[3];
};
layout(std430, set = 0, binding = 2) readonly buffer InputVertexBuffer
{
	Vertex inputVertices[3];
};
layout(std140, set = 0, binding = 3) uniform InputUniformBuffer
{
	vec4 inputColorUniform;
};

// Output
layout(rgba8, set = 0, binding = 4) writeonly uniform image2D OutputTexture2D;
layout(std430, set = 0, binding = 5) writeonly buffer OutputIndexBuffer
{
	uint outputIndices[3];
};
layout(std430, set = 0, binding = 6) writeonly buffer OutputVertexBuffer
{
	Vertex outputVertices[3];
};

// Programs
layout (local_size_x = 16, local_size_y = 16) in;
void main()
{
	// Fetch input texel
	vec4 color = texelFetch(InputTexture2D, ivec2(gl_GlobalInvocationID.xy), 0) * inputColorUniform;

	// Modify color
	color.g *= 1.0f - (float(gl_GlobalInvocationID.x) / 16.0f);
	color.g *= 1.0f - (float(gl_GlobalInvocationID.y) / 16.0f);

	// Output texel
	imageStore(OutputTexture2D, ivec2(gl_GlobalInvocationID.xy), color);

	// Output buffer
	if (0 == gl_GlobalInvocationID.x && 0 == gl_GlobalInvocationID.y && 0 == gl_GlobalInvocationID.z)
	{
		// Output index buffer values
		for (int indexBufferIndex = 0; indexBufferIndex < 3; ++indexBufferIndex)
		{
			outputIndices[indexBufferIndex] = inputIndices[indexBufferIndex];
		}

		// Output vertex buffer values
		for (int vertexBufferIndex = 0; vertexBufferIndex < 3; ++vertexBufferIndex)
		{
			outputVertices[vertexBufferIndex] = inputVertices[vertexBufferIndex];
		}

		// Output uniform buffer not possible by design
	}
}
)";

computeShaderSourceCode2 = R"(#version 450 core	// OpenGL 4.5
struct Vertex
{
	vec2 position;
	vec2 padding;
};

// Same layout as "Renderer::DrawIndexedInstancedArguments"
struct DrawIndexedInstancedArguments
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	uint baseVertexLocation;
	uint startInstanceLocation;
};

// Input
layout(set = 0, binding = 0) uniform samplerBuffer InputTextureBuffer;
layout(std430, set = 0, binding = 1) readonly buffer InputStructuredBuffer
{
	Vertex inputStructuredBufferVertex[];
};
layout(std430, set = 0, binding = 2) readonly buffer InputIndirectBuffer
{
	DrawIndexedInstancedArguments inputDrawIndexedInstancedArguments;
};

// Output
layout(rgba32f, set = 0, binding = 3) writeonly uniform imageBuffer OutputTextureBuffer;
layout(std430, set = 0, binding = 4) writeonly buffer OutputStructuredBuffer
{
	Vertex outputStructuredBufferVertex[];
};
layout(std430, set = 0, binding = 5) writeonly buffer OutputIndirectBuffer
{
	DrawIndexedInstancedArguments outputDrawIndexedInstancedArguments;
};

// Programs
layout (local_size_x = 3, local_size_y = 1) in;
void main()
{
	// Output buffer
	if (0 == gl_GlobalInvocationID.x && 0 == gl_GlobalInvocationID.y && 0 == gl_GlobalInvocationID.z)
	{
		// Output texture buffer values
		for (int textureBufferIndex = 0; textureBufferIndex < 3; ++textureBufferIndex)
		{
			imageStore(OutputTextureBuffer, textureBufferIndex, texelFetch(InputTextureBuffer, textureBufferIndex));
		}

		// Output structured buffer values
		for (int structuredBufferIndex = 0; structuredBufferIndex < 3; ++structuredBufferIndex)
		{
			outputStructuredBufferVertex[structuredBufferIndex] = inputStructuredBufferVertex[structuredBufferIndex];
		}

		// Output indirect buffer values (draw calls)
		// outputDrawIndexedInstancedArguments.indexCountPerInstance = inputDrawIndexedInstancedArguments.indexCountPerInstance;	- Filled by compute shader via atomics counting
		outputDrawIndexedInstancedArguments.instanceCount		  = inputDrawIndexedInstancedArguments.instanceCount;
		outputDrawIndexedInstancedArguments.startIndexLocation	  = inputDrawIndexedInstancedArguments.startIndexLocation;
		outputDrawIndexedInstancedArguments.baseVertexLocation	  = inputDrawIndexedInstancedArguments.baseVertexLocation;
		outputDrawIndexedInstancedArguments.startInstanceLocation = inputDrawIndexedInstancedArguments.startInstanceLocation;
	}

	// Atomics for counting usage example
	// -> Change 'layout (local_size_x = 3, local_size_y = 1) in;' into 'layout (local_size_x = 1, local_size_y = 1) in;' and if the triangle is gone you know the counter reset worked
	if (0 == gl_GlobalInvocationID.x)
	{
		// Reset the counter on first invocation
		atomicExchange(outputDrawIndexedInstancedArguments.indexCountPerInstance, 0);
	}
	atomicAdd(outputDrawIndexedInstancedArguments.indexCountPerInstance, 1);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
