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
#ifdef RENDERER_DIRECT3D9
if (renderer.getNameId() == Renderer::NameId::DIRECT3D9)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(
// Attribute input/output
struct VS_INPUT
{
	// Mesh data
	float3 Position : POSITION;	// Object space vertex position
	float2 TexCoord : TEXCOORD0;
	float3 Normal   : NORMAL;
	// Per-instance data
	float4 PerInstancePositionTexture : TEXCOORD1;	// Position: xyz=Position, w=Slice of the 2D texture array to use
	float4 PerInstanceRotationScale   : TEXCOORD2;	// Rotation: Rotation quaternion (xyz) and scale (w)
													// -> We don't need to store the w component of the quaternion. It's normalized and storing
													//    three components while recomputing the fourths component is be sufficient.
};
struct VS_OUTPUT
{
	float4 Position      : SV_POSITION;
	float2 TexCoord      : TEXCOORD0;
	float3 Normal        : TEXCOORD1;
	float3 WorldPosition : TEXCOORD2;
};

// Uniforms
uniform float4x4 MVP;
uniform float2 TimerAndGlobalScale;	// x=Timer, y=Global scale

// Programs
VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT output;

	// Compute last component (w) of the quaternion (rotation quaternions are always normalized)
	float sqw = 1.0f - Input.PerInstanceRotationScale.x * Input.PerInstanceRotationScale.x
					 - Input.PerInstanceRotationScale.y * Input.PerInstanceRotationScale.y
					 - Input.PerInstanceRotationScale.z * Input.PerInstanceRotationScale.z;
	float4 r = float4(Input.PerInstanceRotationScale.xyz, (sqw > 0.0f) ? -sqrt(sqw) : 0.0f);

	{ // Cube rotation: SLERP from identity quaternion to rotation quaternion of the current instance
		// From
		float4 from = float4(0.0, 0.0, 0.0, 1.0);	// Identity

		// To
		float4 to = r;

		// Time
		float time = TimerAndGlobalScale.x * 0.001f;

		// Calculate cosine
		float cosom = dot(from, to);

		// Adjust signs (if necessary)
		float4 to1;
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
			// Standard case (SLERP)
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
	float4 position = float4(Input.Position, 1.0f);

	{ // Apply rotation by using the rotation quaternion
		float x2 = r.x * r.x;
		float y2 = r.y * r.y;
		float z2 = r.z * r.z;
		float w2 = r.w * r.w;
		float xa = r.x * position.x;
		float yb = r.y * position.y;
		float zc = r.z * position.z;
		position.xyz = float3(position.x * ( x2 - y2 - z2 + w2) + 2.0 * (r.w * (r.y * position.z - r.z * position.y) + r.x * (yb + zc)),
							  position.y * (-x2 + y2 - z2 + w2) + 2.0 * (r.w * (r.z * position.x - r.x * position.z) + r.y * (xa + zc)),
							  position.z * (-x2 - y2 + z2 + w2) + 2.0 * (r.w * (r.x * position.y - r.y * position.x) + r.z * (xa + yb)));
	}

	// Apply global scale and per instance scale
	position.xyz = position.xyz * TimerAndGlobalScale.y * Input.PerInstanceRotationScale.w;

	// Some movement in general
	position.x += sin(TimerAndGlobalScale.x * 0.0001f);
	position.y += sin(TimerAndGlobalScale.x * 0.0001f) * 2.0f;
	position.z += cos(TimerAndGlobalScale.x * 0.0001f) * 0.5f;

	// Apply per instance position
	position.xyz += Input.PerInstancePositionTexture.xyz;

	// Calculate the world position of the vertex
	output.WorldPosition = position.xyz;

	// Calculate the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	position = mul(position, MVP);

	// Write out the final vertex data
	output.Position = position;
	output.TexCoord = float2(Input.TexCoord.x, Input.TexCoord.y / 8.0f + 1.0f / 8.0f * Input.PerInstancePositionTexture.w);	// Fixed build in number of textures
	output.Normal = Input.Normal;

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
// Attribute input
struct VS_OUTPUT
{
	float4 Position      : SV_POSITION;
	float2 TexCoord      : TEXCOORD0;
	float3 Normal        : TEXCOORD1;
	float3 WorldPosition : TEXCOORD2;
};

// Uniforms
uniform sampler2D AlbedoMap : register(s0);
uniform float3 LightPosition;	// World space light position

// Programs
float4 main(VS_OUTPUT Input) : SV_TARGET
{
	// Simple point light by using Lambert's cosine law
	float lighting = clamp(dot(Input.Normal, normalize(LightPosition - Input.WorldPosition)), 0.0f, 0.8f);

	// Calculate the final fragment color
	// -> Direct3D 9 is using BGR, the world is using RGB
	float4 color = (float4(0.2f, 0.2f, 0.2f, 1.0f) + lighting) * tex2D(AlbedoMap, Input.TexCoord).bgra;
	color.a = 0.8f;

	// Done
	return color;
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
