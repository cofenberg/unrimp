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


// Amalgamated/unity build

// Recommended example order

// Basics
	#include "src/Basics/FirstTriangle/FirstTriangle.cpp"
	#include "src/Basics/FirstIndirectBuffer/FirstIndirectBuffer.cpp"
	#include "src/Basics/VertexBuffer/VertexBuffer.cpp"
	#include "src/Basics/FirstTexture/FirstTexture.cpp"
	#include "src/Basics/FirstRenderToTexture/FirstRenderToTexture.cpp"
	#include "src/Basics/FirstMultipleRenderTargets/FirstMultipleRenderTargets.cpp"
	#ifndef __ANDROID__
		#include "src/Basics/FirstMultipleSwapChains/FirstMultipleSwapChains.cpp"
	#endif
	#include "src/Basics/FirstInstancing/FirstInstancing.cpp"
	#include "src/Basics/FirstGeometryShader/FirstGeometryShader.cpp"
	#include "src/Basics/FirstTessellation/FirstTessellation.cpp"

// Advanced
	#include "src/Advanced/FirstGpgpu/FirstGpgpu.cpp"
	#include "src/Advanced/InstancedCubes/CubeRendererDrawInstanced/BatchDrawInstanced.cpp"
	#include "src/Advanced/InstancedCubes/CubeRendererDrawInstanced/CubeRendererDrawInstanced.cpp"
	#include "src/Advanced/InstancedCubes/CubeRendererInstancedArrays/BatchInstancedArrays.cpp"
	#include "src/Advanced/InstancedCubes/CubeRendererInstancedArrays/CubeRendererInstancedArrays.cpp"
	#include "src/Advanced/InstancedCubes/InstancedCubes.cpp"
	#include "src/Advanced/IcosahedronTessellation/IcosahedronTessellation.cpp"

// Runtime
#ifdef RENDERER_RUNTIME
	#ifdef RENDERER_RUNTIME_IMGUI
		#include "src/Runtime/ImGuiExampleSelector/ImGuiExampleSelector.cpp"
	#endif
	#include "src/Runtime/FirstMesh/FirstMesh.cpp"
	#include "src/Runtime/FirstCompositor/FirstCompositor.cpp"
	#include "src/Runtime/FirstCompositor/CompositorInstancePassFirst.cpp"
	#include "src/Runtime/FirstCompositor/CompositorPassFactoryFirst.cpp"
	#include "src/Runtime/FirstScene/FirstScene.cpp"
	#include "src/Runtime/FirstScene/FreeCameraController.cpp"
	#ifdef RENDERER_RUNTIME_OPENVR
		#include "src/Runtime/FirstScene/VrController.cpp"
	#endif
#endif
