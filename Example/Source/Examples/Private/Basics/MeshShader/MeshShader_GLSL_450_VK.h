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
#ifdef RHI_VULKAN
if (rhi->getNameId() == Rhi::NameId::VULKAN)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// Mesh shader
meshShaderSourceCode = R"(#version 450 // OpenGL 4.5

#extension GL_NV_mesh_shader : require
 
layout(local_size_x = 1) in;
layout(triangles, max_vertices = 3, max_primitives = 1) out;

out gl_MeshPerVertexNV
{
	vec4 gl_Position;
} gl_MeshVerticesNV[];

const vec3 vertices[3] = {vec3(0.0f, 1.0f, 0.5), vec3(1.0f, 0.0f, 0.5), vec3(-0.5f, 0.0f, 0.5)};

// From http://zone.dog/braindump/mesh_shaders/
// If we don't redeclare gl_PerVertex, compilation fails with the following error:
// error C7592: ARB_separate_shader_objects requires built-in block gl_PerVertex to be redeclared before accessing its members
/*
out gl_PerVertex
{
	vec4 gl_Position;
} gl_Why;
*/
void main()
{
	// Vertices position
 	gl_MeshVerticesNV[0].gl_Position = vec4(vertices[0], 1.0); 
	gl_MeshVerticesNV[1].gl_Position = vec4(vertices[1], 1.0); 
	gl_MeshVerticesNV[2].gl_Position = vec4(vertices[2], 1.0); 
 
	// Triangle indices
	gl_PrimitiveIndicesNV[0] = 0;
	gl_PrimitiveIndicesNV[1] = 1;
	gl_PrimitiveIndicesNV[2] = 2;
 
	// Number of triangles  
	gl_PrimitiveCountNV = 1;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 450 core	// OpenGL 4.5

// Attribute input/output
layout(location = 0) out vec4 OutputColor;	// Output variable for fragment color

// Programs
void main()
{
	// Return white
	OutputColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
