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


== Description ==
Standalone renderer examples.


== Recommended Example Order ==
- Basics
	- "FirstTriangle" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Debug methods: When using Direct3D <11.1, those methods map to the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)
	- "FirstIndirectBuffer" demonstrates:
		- Everything from "FirstTriangle"
		- Indirect buffer
	- "FirstQueries" demonstrates:
		- Everything from "FirstTriangle"
		- Asynchronous queries
	- "VertexBuffer" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Multiple vertex attributes within a single vertex buffer object (VBO), vertex array object (VAO) is only using one vertex buffer object (VBO)
		- One vertex buffer object (VBO) per vertex attribute, vertex array object (VAO) is using multiple vertex buffer objects (VBO)
	- "FirstTexture" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 1D and 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
	- "FirstRenderToTexture" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
	- "FirstMultipleRenderTargets" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
		- Multiple render targets (MRT)
	- "FirstMultipleSwapChains" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Multiple swap chains
	- "FirstInstancing" demonstrates:
		- Index buffer object (IBO)
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		- Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
	- "FirstGeometryShader" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS), geometry shader (GS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Attribute-less rendering (aka "drawing without data")
	- "FirstTessellationShader" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Vertex shader (VS), tessellation control shader (TCS), tessellation evaluation shader (TES) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
	- "FirstComputeShader" demonstrates:
		- Index buffer object (IBO)
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Texture buffer object (TBO)
		- Structured buffer object
		- Indirect buffer
		- Uniform buffer object (UBO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS), fragment shader (FS) and compute shader (CS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
- Advanced
	- "FirstGpgpu" demonstrates:
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- 2D texture
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Framebuffer object (FBO) used for render to texture
		- General Purpose Computation on Graphics Processing Unit (GPGPU) by using the RHI and shaders without having any output window (also known as headless rendering)
	- "InstancedCubes" demonstrates:
		- Index buffer object (IBO)
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Texture buffer object (TBO)
		- Uniform buffer object (UBO)
		- 2D texture
		- 2D texture array
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		- Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
	- "IcosahedronTessellation" demonstrates:
		- Index buffer object (IBO)
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Uniform buffer object (UBO)
		- Vertex shader (VS), tessellation control shader (TCS), tessellation evaluation shader (TES), geometry shader (GS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
- Renderer
	- "ImGuiExampleSelector"demonstrates:
		- ImGui usage to select the example to start
	- "FirstMesh" demonstrates:
		- Index buffer object (IBO)
		- Vertex buffer object (VBO)
		- Vertex array object (VAO)
		- Texture buffer object (TBO)
		- Uniform buffer object (UBO)
		- Sampler state object
		- Vertex shader (VS) and fragment shader (FS)
		- Root signature
		- Graphics pipeline state object (PSO)
		- Blinn-Phong shading
		- Albedo, normal, roughness and emissive mapping
		- Optimization: Cache data to not bother the RHI to much
		- Compact vertex format (32 bit texture coordinate, 16 bit QTangent, 56 bytes vs. 28 bytes per vertex)
	- "FirstCompositor" demonstrates:
		- Compositor
		- Debug GUI manager usage
	- "FirstScene" demonstrates:
		- Compositor
		- Scene
		- Virtual reality (VR)


== Dependencies ==
- Renderer
- Renderer toolkit for hot-reloading support
- PhysicsFS (directly compiled and linked in)
- Optional SDL2 ( https://www.libsdl.org/ )


== Preprocessor Definitions ==
For supporting a particular RHI implementations:
- "RHI_NULL":		  Enable Null RHI implementation support
- "RHI_VULKAN":		  Enable Vulkan RHI implementation support
- "RHI_OPENGL":		  Enable OpenGL RHI implementation support
- "RHI_OPENGLES3":	  Enable OpenGL ES 3 RHI implementation support
- "RHI_DIRECT3D9":	  Enable Direct3D 9 RHI implementation support
- "RHI_DIRECT3D10":	  Enable Direct3D 10 RHI implementation support
- "RHI_DIRECT3D11":	  Enable Direct3D 11 RHI implementation support
- "RHI_DIRECT3D12":	  Enable Direct3D 12 RHI implementation support
- "UNICODE":		  Enable Microsoft Windows command line Unicode support
- "SHARED_LIBRARIES": Use RHIs via shared libraries, if this is not defined, the RHIs are statically linked
- "RENDERER":		  Enable renderer support
- "RENDERER_TOOLKIT": Enable renderer toolkit support
- "SDL2_FOUND":"	  Enable SDL2 ( https://www.libsdl.org/ ) support
- Do also have a look into the RHI header file for RHI implementation preprocessor definitions
