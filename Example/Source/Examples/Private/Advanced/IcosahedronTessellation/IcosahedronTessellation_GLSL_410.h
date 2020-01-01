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


// Shaders are from: http://prideout.net/blog/?p=48 (Philip Rideout, "The Little Grasshopper - Graphics Programming Tips")


//[-------------------------------------------------------]
//[ Shader start                                          ]
//[-------------------------------------------------------]
#ifdef RHI_OPENGL
if (rhi->getNameId() == Rhi::NameId::OPENGL)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per control point of the patch
vertexShaderSourceCode = R"(#version 410 core	// OpenGL 4.1

// Attribute input/output
in  vec3 Position;	// Object space control point position of the patch we received from the input assembly (IA) as input
out vec3 vPosition;	// Object space control point position of the patch as output

// Programs
void main()
{
	// Pass through the object space control point position of the patch
	vPosition = Position;
}
)";


//[-------------------------------------------------------]
//[ Tessellation control shader source code               ]
//[-------------------------------------------------------]
// One tessellation control shader invocation per patch control point (with super-vision)
tessellationControlShaderSourceCode = R"(#version 410 core	// OpenGL 4.1

// Attribute input/output
in  vec3 vPosition[];	// Object space control point position of the patch we received from the vertex shader (VS) as input
out vec3 tcPosition[];	// Object space control point position of the patch as output

// Uniforms
layout(std140) uniform UniformBlockDynamicTcs
{
	float TessellationLevelOuter;	// Outer tessellation level
	float TessellationLevelInner;	// Inner tessellation level
};

// Programs
layout(vertices = 3) out;
void main()
{
	// Pass through the object space control point position of the patch
	tcPosition[gl_InvocationID] = vPosition[gl_InvocationID];

	// If this is the first control point of the patch, inform the tessellator about the desired tessellation level
	if (0 == gl_InvocationID)
	{
		gl_TessLevelOuter[0] = TessellationLevelOuter;
		gl_TessLevelOuter[1] = TessellationLevelOuter;
		gl_TessLevelOuter[2] = TessellationLevelOuter;
		gl_TessLevelInner[0] = TessellationLevelInner;
	}
}
)";


//[-------------------------------------------------------]
//[ Tessellation evaluation shader source code            ]
//[-------------------------------------------------------]
// One tessellation evaluation shader invocation per point from tessellator
tessellationEvaluationShaderSourceCode = R"(#version 410 core	// OpenGL 4.1

// Attribute input/output
in  vec3 tcPosition[];		// Object space control point position of the patch we received from the tessellation control shader (TCS) as input
out gl_PerVertex
{
	vec4 gl_Position;
};
out vec3 tePosition;		// Interpolated object space vertex position inside the patch as output
out vec3 tePatchDistance;	// The barycentric coordinate inside the patch we received from the tessellator as output

// Uniforms
layout(std140) uniform UniformBlockStaticTes
{
	mat4 ObjectSpaceToClipSpaceMatrix;	// Object space to clip space matrix
};

// Programs
layout(triangles, equal_spacing, ccw) in;
void main()
{
	// The barycentric coordinate "gl_TessCoord" we received from the tessellator defines a location
	// inside a triangle as a combination of the weight of the three control points of the patch

	// Calculate the vertex object space position inside the patch by using the barycentric coordinate
	// we received from the tessellator and the three object space control points of the patch
	vec3 p0 = gl_TessCoord.x * tcPosition[0];
	vec3 p1 = gl_TessCoord.y * tcPosition[1];
	vec3 p2 = gl_TessCoord.z * tcPosition[2];
	tePosition = normalize(p0 + p1 + p2);

	// Pass through the barycentric coordinate inside the patch
	tePatchDistance = gl_TessCoord;

	// Calculate the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	gl_Position = ObjectSpaceToClipSpaceMatrix * vec4(tePosition, 1.0);
}
)";


//[-------------------------------------------------------]
//[ Geometry shader source code                           ]
//[-------------------------------------------------------]
// One geometry shader invocation per primitive
geometryShaderSourceCode = R"(#version 410 core	// OpenGL 4.1

// Attribute input/output
in gl_PerVertex
{
	vec4 gl_Position;
} gl_in[3];
in  vec3 tePosition[3];			// Interpolated object space vertex position inside the patch we received from the tessellation evaluation shader (TES) as input
in  vec3 tePatchDistance[3];	// The barycentric coordinate inside the patch from the tessellator we received from the tessellation evaluation shader (TES) as input
out gl_PerVertex
{
	vec4 gl_Position;
};
out vec3 gFacetNormal;			// Normalized normal of the primitive as output
out vec3 gPatchDistance;		// The barycentric coordinate inside the patch from the tessellator as output
out vec3 gTriDistance;			// Local triangle vertex position as output

// Uniforms
layout(std140) uniform UniformBlockStaticGs
{
	// TODO(co) mat3
	mat4 NormalMatrix;	// Object space to clip space rotation matrix
};

// Programs
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
void main()
{
	vec3 A = tePosition[2] - tePosition[0];
	vec3 B = tePosition[1] - tePosition[0];
	// TODO(co) mat3
	gFacetNormal = (NormalMatrix * vec4(normalize(cross(A, B)), 1.0)).xyz;

	// Emit vertex 0
	gPatchDistance = tePatchDistance[0];
	gTriDistance = vec3(1.0, 0.0, 0.0);
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();

	// Emit vertex 1
	gPatchDistance = tePatchDistance[1];
	gTriDistance = vec3(0.0, 1.0, 0.0);
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();

	// Emit vertex 2
	gPatchDistance = tePatchDistance[2];
	gTriDistance = vec3(0.0, 0.0, 1.0);
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();

	EndPrimitive();
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 410 core	// OpenGL 4.1

// Attributes
in vec3 gFacetNormal;	// Normalized normal of the primitive we received from the geometry shader (GS) as input
in vec3 gPatchDistance;	// The barycentric coordinate inside the patch from the tessellator we received from the geometry shader (GS) as input
in vec3 gTriDistance;	// Local triangle vertex position we received from the geometry shader (GS) as input
layout(location = 0) out vec4 OutputColor;	// Output variable for fragment color

// Uniforms
layout(std140) uniform UniformBlockStaticFs
{
	// TODO(co) vec3
	vec4 LightPosition;
	vec4 DiffuseMaterial;
	vec4 AmbientMaterial;
};

// Programs
float Amplify(float d, float scale, float offset)
{
	d = scale * d + offset;
	d = clamp(d, 0.0, 1.0);
	d = 1.0 - exp2(-2.0 * d * d);
	return d;
}

void main()
{
	// Simple lighting
	vec3 N = normalize(gFacetNormal);
	vec3 L = LightPosition.xyz;// TODO(co) vec3
	float df = abs(dot(N, L));
	vec3 color = AmbientMaterial.rgb + df * DiffuseMaterial.rgb;// TODO(co) vec3

	// Add wireframe via fragment color manipulation
	// -> Thick (60) black lines for the patch edges
	// -> Thin (40) black lines for the edges of the generated triangles inside the patch
	// -> Determine how far the current fragment is from the nearest edge of the patch and
	//    local triangle by use the given interpolated positions inside the patch and local triangle
	float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
	float d2 = min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z);
	color = Amplify(d1, 40.0, -0.5) * Amplify(d2, 60.0, -0.5) * color;

	// Return the calculated color
	OutputColor = vec4(color, 1.0);
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
