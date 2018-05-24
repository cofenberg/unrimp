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
#ifndef RENDERER_NO_OPENGLES3
if (mRenderer->getNameId() == Renderer::NameId::OPENGLES3)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
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
	// -> In OpenGL ES 3, the texture origin is left/bottom which maps well to clip space coordinates
	// -> (-1,-1) -> (0,0)
	// -> (1,1) -> (1,1)
	TexCoord = Position.xy * 0.5 + 0.5;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code - Content generation      ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode_ContentGeneration = R"(#version 300 es	// OpenGL ES 3.0

// Attribute input/output
in  mediump vec2 TexCoord;		// Normalized texture coordinate as input
out highp   vec4 OutputColor;	// Output variable for fragment color

// Programs
void main()
{
	// Return the color green
	OutputColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code - Content processing      ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
fragmentShaderSourceCode_ContentProcessing = R"(#version 300 es	// OpenGL ES 3.0

// Attribute input/output
in  mediump vec2 TexCoord;		// Normalized texture coordinate as input
out highp   vec4 OutputColor;	// Output variable for fragment color

// Uniforms
uniform mediump sampler2D ContentMap;

// Programs
void main()
{
	// Fetch the texel at the given texture coordinate and return its color
	// -> Apply a simple wobble to the texture coordinate so we can see that content processing is up and running
	OutputColor = texture(ContentMap, vec2(TexCoord.x + sin(TexCoord.x * 100.0) * 0.01, TexCoord.y + cos(TexCoord.y * 100.0) * 0.01));
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
