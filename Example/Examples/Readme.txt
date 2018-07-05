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


== Description ==
Standalone renderer examples.


== Recommended Example Order ==
- Basics
	- "FirstTriangle" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Debug methods: When using Direct3D <11.1, those methods map to the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)
	- "FirstIndirectBuffer" demonstrates:
		- Everything from "FirstTriangle"
		- Indirect buffer
	- "VertexBuffer" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Multiple vertex attributes within a single vertex buffer object (VBO), vertex array object (VAO) is only using one vertex buffer object (VBO)
		- One vertex buffer object (VBO) per vertex attribute, vertex array object (VAO) is using multiple vertex buffer objects (VBO)
	- "FirstTexture" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 1D and 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
	- "FirstRenderToTexture" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
	- "FirstMultipleRenderTargets" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
		- Multiple render targets (MRT)
	- "FirstMultipleSwapChains" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Multiple swap chains
	- "FirstInstancing" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Index buffer object (IBO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		- Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
	- "FirstGeometryShader" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS), geometry shader (GS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Attribute-less rendering (aka "drawing without data")
	- "FirstTessellation" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS), tessellation control shader (TCS), tessellation evaluation shader (TES) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
	- "FirstComputeShader" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS), fragment shader (FS) and compute shader (CS)
		- Root signature
		- Pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
- Advanced
	- "FirstGpgpu" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
		- General Purpose Computation on Graphics Processing Unit (GPGPU) by using the renderer interface and shaders without having any output window
	- "InstancedCubes" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Index buffer object (IBO)
		- Uniform buffer object (UBO)
		- Texture buffer object (TBO)
		- 2D texture
		- 2D texture array
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		- Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
	- "IcosahedronTessellation" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Index buffer object (IBO)
		- Uniform buffer object (UBO)
		- Vertex shader (VS), tessellation control shader (TCS), tessellation evaluation shader (TES), geometry shader (GS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
- Runtime
	- "ImGuiExampleSelector"demonstrates:
		- ImGui usage to select the example to start
	- "FirstMesh" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Index buffer object (IBO)
		- Uniform buffer object (UBO)
		- Texture buffer object (TBO)
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Pipeline state object (PSO)
		- Blinn-Phong shading
		- Albedo, normal, roughness and emissive mapping
		- Optimization: Cache data to not bother the renderer API to much
		- Compact vertex format (32 bit texture coordinate, 16 bit QTangent, 56 bytes vs. 28 bytes per vertex)
	- "FirstCompositor" demonstrates:
		- Compositor
		- Debug GUI manager usage
	- "FirstScene" demonstrates:
		- Compositor
		- Scene
		- Virtual reality (VR)


== Dependencies ==
- Renderer runtime
- Renderer toolkit for hot-reloading support
- PhysicsFS (directly compiled and linked in)
- Optional SDL2 ( https://www.libsdl.org/ )


== Preprocessor Definitions ==
For supporting a particular renderer backend:
- "RENDERER_NULL":		 Enable Null renderer backend support
- "RENDERER_VULKAN":	 Enable Vulkan renderer backend support
- "RENDERER_OPENGL":	 Enable OpenGL renderer backend support
- "RENDERER_OPENGLES3":	 Enable OpenGL ES 3 renderer backend support
- "RENDERER_DIRECT3D9":	 Enable Direct3D 9 renderer backend support
- "RENDERER_DIRECT3D10": Enable Direct3D 10 renderer backend support
- "RENDERER_DIRECT3D11": Enable Direct3D 11 renderer backend support
- "RENDERER_DIRECT3D12": Enable Direct3D 12 renderer backend support
- "UNICODE":			 Enable Microsoft Windows command line Unicode support
- "SHARED_LIBRARIES":	 Use renderers via shared libraries, if this is not defined, the renderers are statically linked
- "RENDERER_RUNTIME":	 Enable renderer runtime support
- "RENDERER_TOOLKIT":	 Enable renderer toolkit support
- "SDL2_FOUND":"		 Enable SDL2 ( https://www.libsdl.org/ ) support
- Do also have a look into the renderer header file for renderer backend preprocessor definitions
