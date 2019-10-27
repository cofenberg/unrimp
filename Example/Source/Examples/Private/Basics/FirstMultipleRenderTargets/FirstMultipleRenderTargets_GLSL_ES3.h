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
#ifdef RHI_OPENGLES3
if (rhi->getNameId() == Rhi::NameId::OPENGLES3)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
if (rhi->getCapabilities().upperLeftOrigin)
{
	// One vertex shader invocation per vertex
	vertexShaderSourceCode = R"(#version 300 es	// OpenGL ES 3.0

	// Attribute input/output
	in  highp vec2 Position;	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
	out highp vec2 TexCoord;	// Normalized texture coordinate as output

	// Programs
	void main()
	{
		// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
		gl_Position = vec4(Position, 0.5, 1.0);

		// Calculate the texture coordinate by mapping the clip space coordinate to a texture space coordinate
		// -> In OpenGL ES 3 with "GL_EXT_clip_control"-extension, the texture origin is left/top which does not map well to clip space coordinates
		// -> We have to flip the y-axis to map the coordinate system to the texture coordinate system
		// -> (-1,-1) -> (0,1)
		// -> (1,1) -> (1,0)
		TexCoord = vec2(Position.x * 0.5f + 0.5f, 1.0f - (Position.y * 0.5f + 0.5f));
	}
	)";
}
else
{
	// One vertex shader invocation per vertex
	vertexShaderSourceCode = R"(#version 300 es	// OpenGL ES 3.0

	// Attribute input/output
	in  highp vec2 Position;	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
	out highp vec2 TexCoord;	// Normalized texture coordinate as output

	// Programs
	void main()
	{
		// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
		gl_Position = vec4(Position, 0.5, 1.0);

		// Calculate the texture coordinate by mapping the clip space coordinate to a texture space coordinate
		// -> In OpenGL ES 3 without "GL_EXT_clip_control"-extension, the texture origin is left/bottom which maps well to clip space coordinates
		// -> (-1,-1) -> (0,0)
		// -> (1,1) -> (1,1)
		TexCoord = Position.xy * 0.5 + 0.5;
	}
	)";
}


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode_MultipleRenderTargets = R"(#version 300 es	// OpenGL ES 3.0
precision highp float; // Default precision to high for floating points

// Attribute input/output
in  mediump vec2 TexCoord;			// Normalized texture coordinate as input
out highp   vec4 OutputColor[2];	// Output variable for fragment color

// Programs
void main()
{
	OutputColor[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);	// Red
	OutputColor[1] = vec4(0.0f, 0.0f, 1.0f, 0.0f);	// Blue
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode = R"(#version 300 es	// OpenGL ES 3.0
precision highp float; // Default precision to high for floating points

// Attribute input/output
in  mediump vec2 TexCoord;		// Normalized texture coordinate as input
out highp   vec4 OutputColor;	// Output variable for fragment color

// Uniforms
uniform mediump sampler2D AlbedoMap0;
uniform mediump sampler2D AlbedoMap1;

// Programs
void main()
{
	// Fetch the texel at the given texture coordinate from render target 0 (which should contain a red triangle)
	vec4 color0 = texture(AlbedoMap0, TexCoord);

	// Fetch the texel at the given texture coordinate from render target 1 (which should contain a blue triangle)
	vec4 color1 = texture(AlbedoMap1, TexCoord);

	// Calculate the final color by subtracting the colors of the both render targets from white
	// -> The result should be white or green
	OutputColor = vec4(1.0, 1.0, 1.0, 1.0) - color0 - color1;
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
