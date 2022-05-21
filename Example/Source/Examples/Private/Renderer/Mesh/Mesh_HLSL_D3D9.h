/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#ifdef RHI_DIRECT3D9
if (rhi->getNameId() == Rhi::NameId::DIRECT3D9)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(
// Attribute input/output
struct VS_INPUT
{
	float3 Position : POSITION;		// Object space vertex position
	float2 TexCoord : TEXCOORD0;	// 32 bit texture coordinate
	float4 QTangent : TEXCOORD1;	// 16 bit QTangent
};
struct VS_OUTPUT
{
	float4   Position     : SV_POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
	float2   TexCoord     : TEXCOORD0;		// Texture coordinate
	float3x3 TangentFrame : TEXCOORD1;		// Tangent frame
};

// Uniforms
uniform float4x4 ObjectSpaceToClipSpaceMatrix;	// Object space to clip space matrix
uniform float3x3 ObjectSpaceToViewSpaceMatrix;	// Object space to view space matrix

// Functions
float2x3 QuaternionToTangentBitangent(float4 q)
{
	return float2x3(
		1 - 2*(q.y*q.y + q.z*q.z),	2*(q.x*q.y + q.w*q.z),		2*(q.x*q.z - q.w*q.y),
		2*(q.x*q.y - q.w*q.z),		1 - 2*(q.x*q.x + q.z*q.z),	2*(q.y*q.z + q.w*q.x)
	);
}

float3x3 GetTangentFrame(float3x3 objectSpaceToViewSpaceMatrix, float4 qtangent)
{
	float2x3 tBt = QuaternionToTangentBitangent(qtangent);
	float3 t = mul(tBt[0], objectSpaceToViewSpaceMatrix);
	float3 b = mul(tBt[1], objectSpaceToViewSpaceMatrix);

	return float3x3(
		t,
		b,
		cross(t, b) * (qtangent.w < 0 ? -1 : 1)
	);
}

// Programs
VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// Calculate the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	output.Position = mul(float4(input.Position, 1.0f), ObjectSpaceToClipSpaceMatrix);

	// Pass through the 32 bit texture coordinate
	output.TexCoord = input.TexCoord;

	// Calculate the tangent space to view space tangent, binormal and normal
	// - 16 bit QTangent basing on http://dev.theomader.com/qtangents/ "QTangents" which is basing on
	//   http://www.crytek.com/cryengine/presentations/spherical-skinning-with-dual-quaternions-and-qtangents "Spherical Skinning with Dual-Quaternions and QTangents"
	output.TangentFrame = GetTangentFrame(ObjectSpaceToViewSpaceMatrix, input.QTangent / 32767.0);

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
// Attribute input/output
struct VS_OUTPUT
{
	float4   Position     : SV_POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
	float2   TexCoord     : TEXCOORD0;		// Texture coordinate
	float3x3 TangentFrame : TEXCOORD1;		// Tangent frame
};

// Uniforms
uniform sampler2D _argb_nxa     : register(s0);
uniform sampler2D _hr_rg_mb_nya : register(s1);
uniform sampler2D EmissiveMap   : register(s2);

// Programs
float4 main(VS_OUTPUT input) : SV_TARGET
{
	// Build in variables
	float3 ViewSpaceLightDirection = normalize(float3(0.5f, 0.5f, -1.0f));	// View space light direction
	float3 ViewSpaceViewVector     = float3(0.0f, 0.0f, -1.0f);				// In view space, we always look along the negative z-axis

	// Read channel packed texture data
	// -> "_argb_nxa" = RGB channel = Albedo map ("_a"-postfix), A channel = x component of normal map ("_n"-postfix)
	// -> "_hr_rg_mb_nya" = R channel = Height map ("_h"-postfix), G channel = Roughness map ("_r"-postfix), B channel = Metallic map ("_m"-postfix), A channel = y component of normal map ("_n"-postfix)
	float4 value_argb_nxa = tex2D(_argb_nxa, input.TexCoord);
	float4 value_hr_rg_mb_nya = tex2D(_hr_rg_mb_nya, input.TexCoord);

	// Get the per fragment normal [0..1] by using a tangent space BC5/3DC/ATI2N stored normal map
	// -> See "Real-Time Normal Map DXT Compression" -> "3.3 Tangent-Space 3Dc" - http://www.nvidia.com/object/real-time-normal-map-dxt-compression.html
	float3 normal;
	normal.x = value_argb_nxa.a * 2.0f - 1.0f;
	normal.y = value_hr_rg_mb_nya.a * 2.0f - 1.0f;
	normal.z = sqrt(1.0f - dot(normal.xy, normal.xy));

	// Transform the tangent space normal into view space
	normal = normalize(mul(normal, input.TangentFrame));

	// Perform standard Blinn-Phong diffuse and specular lighting

	// Calculate the diffuse lighting
	float diffuseLight = max(dot(normal, ViewSpaceLightDirection), 0.0f);

	// Calculate the specular lighting
	float3 viewSpaceHalfVector = normalize(ViewSpaceLightDirection + ViewSpaceViewVector);
	float specularLight = (diffuseLight > 0.0f) ? pow(max(dot(normal, viewSpaceHalfVector), 0.0f), 128.0f) : 0.0f;

	// Calculate the fragment color
	float4 color = diffuseLight * float4(value_argb_nxa.rgb, 1.0f);		// Diffuse term
	color.rgb += specularLight * value_hr_rg_mb_nya.g;					// Specular term
	color.rgb += tex2D(EmissiveMap, input.TexCoord).rgb;				// Emissive term

	// Done
	return min(color, float4(1.0f, 1.0f, 1.0f, 1.0f));
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
