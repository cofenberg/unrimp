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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "PrecompiledHeader.h"
#include "ExampleRunner.h"
#include "Framework/PlatformTypes.h"
#include "Framework/CommandLineArguments.h"
#include "Framework/IApplicationRenderer.h"
#include "Framework/IApplicationRendererRuntime.h"
// Basics
#include "Basics/FirstTriangle/FirstTriangle.h"
#include "Basics/FirstIndirectBuffer/FirstIndirectBuffer.h"
#include "Basics/VertexBuffer/VertexBuffer.h"
#include "Basics/FirstTexture/FirstTexture.h"
#include "Basics/FirstRenderToTexture/FirstRenderToTexture.h"
#include "Basics/FirstMultipleRenderTargets/FirstMultipleRenderTargets.h"
#include "Basics/FirstMultipleSwapChains/FirstMultipleSwapChains.h"
#include "Basics/FirstInstancing/FirstInstancing.h"
#include "Basics/FirstGeometryShader/FirstGeometryShader.h"
#include "Basics/FirstTessellation/FirstTessellation.h"
// Advanced
#include "Advanced/FirstGpgpu/FirstGpgpu.h"
#include "Advanced/IcosahedronTessellation/IcosahedronTessellation.h"
#ifndef RENDERER_NO_RUNTIME
	#include "Runtime/FirstMesh/FirstMesh.h"
	#include "Runtime/FirstCompositor/FirstCompositor.h"
	#include "Runtime/FirstScene/FirstScene.h"
	#include "Advanced/InstancedCubes/InstancedCubes.h"
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <array>
	#include <algorithm>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Helper function templates                             ]
//[-------------------------------------------------------]
template <class ExampleClass>
int RunRenderExample(const char* rendererName)
{
	ExampleClass example;
	return IApplicationRenderer(rendererName, &example).run();
}

template <class ExampleClass>
int RunRenderRuntimeExample(const char* rendererName)
{
	ExampleClass example;
	return IApplicationRendererRuntime(rendererName, &example).run();
}

template <class ExampleClass>
int RunExample(const char* rendererName)
{
	return ExampleClass(rendererName).run();
}

ExampleRunner::ExampleRunner() :
	// Case sensitive name of the renderer to instance, might be ignored in case e.g. "RENDERER_ONLY_DIRECT3D12" was set as preprocessor definition
	// -> Example renderer names: "Null", "OpenGL", "OpenGLES3", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	// -> In case the graphics driver supports it, the OpenGL ES 3 renderer can automatically also run on a desktop PC without an emulator (perfect for testing/debugging)
	m_defaultRendererName(
		#ifdef RENDERER_ONLY_NULL
			"Null"
		#elif defined(RENDERER_ONLY_OPENGL) || defined(LINUX)
			"OpenGL"
		#elifdef RENDERER_ONLY_OPENGLES3
			"OpenGLES3"
		#elifdef RENDERER_ONLY_VULKAN
			"Vulkan"
		#elif WIN32
			#ifdef RENDERER_ONLY_DIRECT3D9
				"Direct3D9"
			#elifdef RENDERER_ONLY_DIRECT3D10
				"Direct3D10"
			#elifdef RENDERER_ONLY_DIRECT3D11
				"Direct3D11"
			#elifdef RENDERER_ONLY_DIRECT3D12
				"Direct3D12"
			#endif
		#endif
	)
{
	// Try to ensure that there's always a default renderer backend in case it's not provided via command line arguments
	if (m_defaultRendererName.empty())
	{
		#ifdef WIN32
			#if !defined(RENDERER_ONLY_NULL) && !defined(RENDERER_ONLY_OPENGL) && !defined(RENDERER_ONLY_OPENGLES3) && !defined(RENDERER_ONLY_DIRECT3D9) && !defined(RENDERER_ONLY_DIRECT3D10) && !defined(RENDERER_ONLY_DIRECT3D12) && !defined(RENDERER_ONLY_VULKAN)
				m_defaultRendererName = "Direct3D11";
			#endif
		#else
			#if !defined(RENDERER_ONLY_NULL) && !defined(RENDERER_ONLY_OPENGLES3) && !defined(RENDERER_ONLY_DIRECT3D9) && !defined(RENDERER_ONLY_DIRECT3D10) && !defined(RENDERER_ONLY_DIRECT3D11) && !defined(RENDERER_ONLY_DIRECT3D12) && !defined(RENDERER_ONLY_VULKAN)
				m_defaultRendererName = "OpenGL";
			#endif
		#endif
	}

	// Sets of supported renderer backends
	std::array<std::string, 8> supportsAllRenderer = {{"Null", "OpenGL", "OpenGLES3", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"}};
	std::array<std::string, 7> doesNotSupportOpenGLES3 = {{"Null", "OpenGL", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"}};
	std::array<std::string, 6> onlyShaderModel4Plus = {{"Null", "OpenGL", "Vulkan", "Direct3D10", "Direct3D11", "Direct3D12"}};
	std::array<std::string, 5> onlyShaderModel5Plus = {{"Null", "OpenGL", "Vulkan", "Direct3D11", "Direct3D12"}};

	// Basics
	addExample("FirstTriangle",					&RunRenderExample<FirstTriangle>,				supportsAllRenderer);
	addExample("FirstIndirectBuffer",			&RunRenderExample<FirstIndirectBuffer>,			supportsAllRenderer);
	addExample("VertexBuffer",					&RunRenderExample<VertexBuffer>,				supportsAllRenderer);
	addExample("FirstTexture",					&RunRenderExample<FirstTexture>,				supportsAllRenderer);
	addExample("FirstRenderToTexture",			&RunRenderExample<FirstRenderToTexture>,		supportsAllRenderer);
	addExample("FirstMultipleRenderTargets",	&RunRenderExample<FirstMultipleRenderTargets>,	supportsAllRenderer);
	addExample("FirstMultipleSwapChains",		&RunExample<FirstMultipleSwapChains>,			supportsAllRenderer);
	addExample("FirstInstancing",				&RunRenderExample<FirstInstancing>,				supportsAllRenderer);
	addExample("FirstGeometryShader",			&RunRenderExample<FirstGeometryShader>,			onlyShaderModel4Plus);
	addExample("FirstTessellation",				&RunRenderExample<FirstTessellation>,			onlyShaderModel5Plus);

	// Advanced
	addExample("FirstGpgpu",					&RunExample<FirstGpgpu>,						supportsAllRenderer);
	addExample("IcosahedronTessellation",		&RunRenderExample<IcosahedronTessellation>,		onlyShaderModel5Plus);
	#ifdef RENDERER_NO_RUNTIME
		m_defaultExampleName = "FirstTriangle";
	#else
		// Renderer runtime
		addExample("FirstMesh",					&RunRenderRuntimeExample<FirstMesh>,		supportsAllRenderer);
		addExample("FirstCompositor",			&RunRenderRuntimeExample<FirstCompositor>,	supportsAllRenderer);
		addExample("FirstScene",				&RunRenderRuntimeExample<FirstScene>,		supportsAllRenderer);
		addExample("InstancedCubes",			&RunRenderRuntimeExample<InstancedCubes>,	supportsAllRenderer);
		m_defaultExampleName = "FirstScene";
	#endif

	#ifndef RENDERER_NO_NULL
		m_availableRenderer.insert("Null");
	#endif
	#ifdef WIN32
		#ifndef RENDERER_NO_DIRECT3D9
			m_availableRenderer.insert("Direct3D9");
		#endif
		#ifndef RENDERER_NO_DIRECT3D10
			m_availableRenderer.insert("Direct3D10");
		#endif
		#ifndef RENDERER_NO_DIRECT3D11
			m_availableRenderer.insert("Direct3D11");
		#endif
		#ifndef RENDERER_NO_DIRECT3D12
			m_availableRenderer.insert("Direct3D12");
		#endif
	#endif
	#ifndef RENDERER_NO_OPENGL
		m_availableRenderer.insert("OpenGL");
	#endif
	#ifndef RENDERER_NO_OPENGLES3
		m_availableRenderer.insert("OpenGLES3");
	#endif
	#ifndef RENDERER_NO_VULKAN
		m_availableRenderer.insert("Vulkan");
	#endif
}

int ExampleRunner::runExample(const std::string& rendererName, const std::string& exampleName)
{
	const std::string& selectedExampleName = exampleName.empty() ? m_defaultExampleName : exampleName;
	AvailableExamplesMap::iterator example = m_availableExamples.find(selectedExampleName);
	AvailableRendererMap::iterator renderer = m_availableRenderer.find(rendererName);
	ExampleToSupportedRendererMap::iterator supportedRenderer = m_supportedRendererForExample.find(selectedExampleName);
	bool rendererNotSupportedByExample = false;
	if (m_supportedRendererForExample.end() != supportedRenderer)
	{
		const SupportedRenderers& supportedRendererList = supportedRenderer->second;
		rendererNotSupportedByExample =  std::find(supportedRendererList.begin(), supportedRendererList.end(), rendererName) == supportedRendererList.end();
	}

	if (m_availableExamples.end() == example || m_availableRenderer.end() == renderer || rendererNotSupportedByExample)
	{
		if (m_availableExamples.end() == example)
			showError("no or unknown example given");
		if (m_availableRenderer.end() == renderer)
			showError("unknown renderer: \"" + rendererName + "\"");
		if (rendererNotSupportedByExample) 
			showError("the example \"" + selectedExampleName + "\" doesn't support renderer: \"" + rendererName + "\"");

		// Print usage
		printUsage(m_availableExamples, m_availableRenderer);
		return 0;
	}
	else
	{
		// Run example
		return example->second(rendererName.c_str());
	}
}

template<typename T>
void ExampleRunner::addExample(const std::string& name, RunnerMethod runnerMethod, T const& supportedRendererList)
{
	m_availableExamples.insert(std::pair<std::string,RunnerMethod>(name, runnerMethod));
	SupportedRenderers supportedRenderers;
	for (const std::string& supportedRenderer : supportedRendererList)
	{
		supportedRenderers.push_back(supportedRenderer);
	}
	m_supportedRendererForExample.insert(std::pair<std::string, std::vector<std::string>>(name, std::move(supportedRenderers)));
}
