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
#ifdef RENDERER_VULKAN
if (renderer->getNameId() == Renderer::NameId::VULKAN)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per control point of the patch
vertexShaderSourceCode = R"(#version 450 core	// OpenGL 4.5

// Attribute input/output
layout(location = 0) in vec2 Position;		// Clip space control point position of the patch as input, left/bottom is (-1,-1) and right/top is (1,1)
layout(location = 0) out vec2 vPosition;	// Clip space control point position of the patch as output, left/bottom is (-1,-1) and right/top is (1,1)

// Programs
void main()
{
	// Pass through the clip space control point position of the patch, left/bottom is (-1,-1) and right/top is (1,1)
	vPosition = Position;
}
)";


//[-------------------------------------------------------]
//[ Tessellation control shader source code               ]
//[-------------------------------------------------------]
// One tessellation control shader invocation per patch control point (with super-vision)
tessellationControlShaderSourceCode = R"(#version 450 core	// OpenGL 4.5

// Attribute input/output
layout(location = 0) in vec2 vPosition[];	// Clip space control point position of the patch we received from the vertex shader (VS) as input
layout(location = 0) out vec2 tcPosition[];	// Clip space control point position of the patch as output

// Programs
layout(vertices = 3) out;
void main()
{
	// Pass through the clip space control point position of the patch
	tcPosition[gl_InvocationID] = vPosition[gl_InvocationID];

	// If this is the first control point of the patch, inform the tessellator about the desired tessellation level
	if (0 == gl_InvocationID)
	{
		gl_TessLevelOuter[0] = 1.0;
		gl_TessLevelOuter[1] = 2.0;
		gl_TessLevelOuter[2] = 3.0;
		gl_TessLevelInner[0] = 4.0;
	}
}
)";


//[-------------------------------------------------------]
//[ Tessellation evaluation shader source code            ]
//[-------------------------------------------------------]
// One tessellation evaluation shader invocation per point from tessellator
tessellationEvaluationShaderSourceCode = R"(#version 450 core	// OpenGL 4.5

// Attribute input/output
layout(location = 0) in vec2 tcPosition[];	// Clip space control point position of the patch we received from the tessellation control shader (TCS) as input
layout(location = 0) out gl_PerVertex
{
	vec4 gl_Position;
};

// Programs
layout(triangles, equal_spacing, ccw) in;
void main()
{
	// The barycentric coordinate "gl_TessCoord" we received from the tessellator defines a location
	// inside a triangle as a combination of the weight of the three control points of the patch

	// Calculate the vertex clip space position inside the patch by using the barycentric coordinate
	// we received from the tessellator and the three clip  space control points of the patch
	// -> Compensate for different Vulkan coordinate system
	vec2 p0 = gl_TessCoord.x * tcPosition[2];
	vec2 p1 = gl_TessCoord.y * tcPosition[1];
	vec2 p2 = gl_TessCoord.z * tcPosition[0];

	// Calculate the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	gl_Position = vec4(p0 + p1 + p2, 0.5f, 1.0);
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 450 core	// OpenGL 4.5

// Attribute input/output
layout(location = 0, index = 0) out vec4 Color0;

// Programs
void main()
{
	// Return white
	Color0 = vec4(1.0, 1.0, 1.0, 1.0);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
