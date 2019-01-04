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
#ifdef RENDERER_OPENGL
if (renderer.getNameId() == Renderer::NameId::OPENGL && 0 == mRenderer->getCapabilities().maximumUniformBufferSize)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(#version 130	// OpenGL 3.0

// Attribute input - Mesh data
in vec3 Position;	// Object space vertex position input
in vec2 TexCoord;
in vec3 Normal;

// Attribute input - Per-instance data
in vec4 PerInstancePositionTexture;	// Position: xyz=Position, w=Slice of the 2D texture array to use
in vec4 PerInstanceRotationScale;	// Rotation: Rotation quaternion (xyz) and scale (w)
									// -> We don't need to store the w component of the quaternion. It's normalized and storing
									//    three components while recomputing the fourths component is be sufficient.

// Attribute output
out vec3 WorldPositionVs;
out vec2 TexCoordVs;
out vec3 NormalVs;

// Uniforms
uniform mat4 MVP;
uniform vec2 TimerAndGlobalScale;	// x=Timer, y=Global scale

// Programs
void main()
{
	// Compute last component (w) of the quaternion (rotation quaternions are always normalized)
	float sqw = 1.0 - PerInstanceRotationScale.x * PerInstanceRotationScale.x
					- PerInstanceRotationScale.y * PerInstanceRotationScale.y
					- PerInstanceRotationScale.z * PerInstanceRotationScale.z;
	vec4 r = vec4(PerInstanceRotationScale.xyz, (sqw > 0.0) ? -sqrt(sqw) : 0.0);

	{ // Cube rotation: SLERP from identity quaternion to rotation quaternion of the current instance
		// From
		vec4 from = vec4(0.0, 0.0, 0.0, 1.0);	// Identity

		// To
		vec4 to = r;

		// Time
		float time = TimerAndGlobalScale.x * 0.001f;

		// Calculate cosine
		float cosom = dot(from, to);

		// Adjust signs (if necessary)
		vec4 to1;
		if (cosom < 0.0f)
		{
			cosom  = -cosom;
			to1 = -to;
		}
		else
		{
			to1 = to;
		}

		// Calculate coefficients
		float scale0;
		float scale1;
		if ((1.0f - cosom) > 0.000001f)
		{
			// Standard case (slerp)
			float omega = acos(cosom);
			float sinom = sin(omega);
			scale0 = sin((1.0f - time) * omega) / sinom;
			scale1 = sin(time * omega) / sinom;
		}
		else
		{
			// "from" and "to" quaternions are very close
			//  ... so we can do a linear interpolation:
			scale0 = 1.0f - time;
			scale1 = time;
		}

		// Calculate final values
		r = scale0 * from + scale1 * to1;
	}

	// Start with the local space vertex position
	vec4 position = vec4(Position, 1.0);

	{ // Apply rotation by using the rotation quaternion
		float x2 = r.x * r.x;
		float y2 = r.y * r.y;
		float z2 = r.z * r.z;
		float w2 = r.w * r.w;
		float xa = r.x * position.x;
		float yb = r.y * position.y;
		float zc = r.z * position.z;
		position.xyz = vec3(position.x * ( x2 - y2 - z2 + w2) + 2.0 * (r.w * (r.y * position.z - r.z * position.y) + r.x * (yb + zc)),
							position.y * (-x2 + y2 - z2 + w2) + 2.0 * (r.w * (r.z * position.x - r.x * position.z) + r.y * (xa + zc)),
							position.z * (-x2 - y2 + z2 + w2) + 2.0 * (r.w * (r.x * position.y - r.y * position.x) + r.z * (xa + yb)));
	}

	// Apply global scale and per instance scale
	position.xyz = position.xyz * TimerAndGlobalScale.y * PerInstanceRotationScale.w;

	// Some movement in general
	position.x += sin(TimerAndGlobalScale.x * 0.0001);
	position.y += sin(TimerAndGlobalScale.x * 0.0001) * 2.0;
	position.z += cos(TimerAndGlobalScale.x * 0.0001) * 0.5;

	// Apply per instance position
	position.xyz += PerInstancePositionTexture.xyz;

	// Calculate the world position of the vertex
	WorldPositionVs = position.xyz;

	// Calculate the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	position = MVP * position;

	// Write out the final vertex data
	gl_Position = position;
	TexCoordVs = vec2(TexCoord.x, TexCoord.y / 8.0 + 1.0 / 8.0 * PerInstancePositionTexture.w);	// Fixed build in number of textures
	NormalVs = Normal;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 130	// OpenGL 3.0

// Attribute input/output
in vec3 WorldPositionVs;
in vec2 TexCoordVs;
in vec3 NormalVs;

// Uniforms
uniform sampler2D AlbedoMap;		// Usage of 'layout(binding = 1)' would be nice, but requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension
uniform vec3      LightPosition;	// World space light position

// Programs
void main()
{
	// Simple point light by using Lambert's cosine law
	float lighting = clamp(dot(NormalVs, normalize(LightPosition - WorldPositionVs)), 0.0, 0.8);

	// Calculate the final fragment color
	gl_FragColor = (vec4(0.2, 0.2, 0.2, 1.0) + lighting) * texture2D(AlbedoMap, TexCoordVs);
	gl_FragColor.a = 0.8;
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
