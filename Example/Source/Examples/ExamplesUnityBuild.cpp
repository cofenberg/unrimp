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


// Amalgamated/unity build

// Recommended example order

// Basics
	#include "Private/Basics/Triangle/Triangle.cpp"
	#include "Private/Basics/IndirectBuffer/IndirectBuffer.cpp"
	#include "Private/Basics/Queries/Queries.cpp"
	#include "Private/Basics/VertexBuffer/VertexBuffer.cpp"
	#include "Private/Basics/Texture/Texture.cpp"
	#include "Private/Basics/CubeTexture/CubeTexture.cpp"
	#include "Private/Basics/RenderToTexture/RenderToTexture.cpp"
	#include "Private/Basics/MultipleRenderTargets/MultipleRenderTargets.cpp"
	#ifndef __ANDROID__
		#include "Private/Basics/MultipleSwapChains/MultipleSwapChains.cpp"
	#endif
	#include "Private/Basics/Instancing/Instancing.cpp"
	#include "Private/Basics/GeometryShader/GeometryShader.cpp"
	#include "Private/Basics/TessellationShader/TessellationShader.cpp"
	#include "Private/Basics/ComputeShader/ComputeShader.cpp"
	#include "Private/Basics/MeshShader/MeshShader.cpp"

// Advanced
	#include "Private/Advanced/Gpgpu/Gpgpu.cpp"
	#include "Private/Advanced/InstancedCubes/CubeRendererDrawInstanced/BatchDrawInstanced.cpp"
	#include "Private/Advanced/InstancedCubes/CubeRendererDrawInstanced/CubeRendererDrawInstanced.cpp"
	#include "Private/Advanced/InstancedCubes/CubeRendererInstancedArrays/BatchInstancedArrays.cpp"
	#include "Private/Advanced/InstancedCubes/CubeRendererInstancedArrays/CubeRendererInstancedArrays.cpp"
	#include "Private/Advanced/InstancedCubes/InstancedCubes.cpp"
	#include "Private/Advanced/IcosahedronTessellation/IcosahedronTessellation.cpp"

// Renderer
#ifdef RENDERER
	#ifdef RENDERER_IMGUI
		#include "Private/Renderer/ImGuiExampleSelector/ImGuiExampleSelector.cpp"
	#endif
	#include "Private/Renderer/Mesh/Mesh.cpp"
	#include "Private/Renderer/Compositor/Compositor.cpp"
	#include "Private/Renderer/Compositor/CompositorInstancePass.cpp"
	#include "Private/Renderer/Compositor/CompositorPassFactory.cpp"
	#include "Private/Renderer/Scene/Scene.cpp"
	#include "Private/Renderer/Scene/FreeCameraController.cpp"
	#ifdef RENDERER_OPENVR
		#include "Private/Renderer/Scene/VrController.cpp"
	#endif
#endif
