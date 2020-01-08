/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Examples/Private/ExampleRunner.h"
#include "Examples/Private/Framework/PlatformTypes.h"
#include "Examples/Private/Framework/CommandLineArguments.h"
#include "Examples/Private/Framework/IApplicationRhi.h"
#include "Examples/Private/Framework/IApplicationRenderer.h"
// Basics
#include "Examples/Private/Basics/Triangle/Triangle.h"
#include "Examples/Private/Basics/IndirectBuffer/IndirectBuffer.h"
#include "Examples/Private/Basics/Queries/Queries.h"
#include "Examples/Private/Basics/VertexBuffer/VertexBuffer.h"
#include "Examples/Private/Basics/Texture/Texture.h"
#include "Examples/Private/Basics/CubeTexture/CubeTexture.h"
#include "Examples/Private/Basics/RenderToTexture/RenderToTexture.h"
#include "Examples/Private/Basics/MultipleRenderTargets/MultipleRenderTargets.h"
#ifndef __ANDROID__
	#include "Examples/Private/Basics/MultipleSwapChains/MultipleSwapChains.h"
#endif
#include "Examples/Private/Basics/Instancing/Instancing.h"
#include "Examples/Private/Basics/GeometryShader/GeometryShader.h"
#include "Examples/Private/Basics/TessellationShader/TessellationShader.h"
#include "Examples/Private/Basics/ComputeShader/ComputeShader.h"
// Advanced
#include "Examples/Private/Advanced/Gpgpu/Gpgpu.h"
#include "Examples/Private/Advanced/IcosahedronTessellation/IcosahedronTessellation.h"
#include "Examples/Private/Advanced/InstancedCubes/InstancedCubes.h"
// Renderer
#ifdef RENDERER
	#ifdef RENDERER_IMGUI
		#include "Examples/Private/Renderer/ImGuiExampleSelector/ImGuiExampleSelector.h"
	#endif
	#include "Examples/Private/Renderer/Mesh/Mesh.h"
	#include "Examples/Private/Renderer/Compositor/Compositor.h"
	#include "Examples/Private/Renderer/Scene/Scene.h"
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
	#include <iostream>
	#include <algorithm>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
ExampleRunner::ExampleRunner() :
	// Case sensitive name of the RHI to instance, might be ignored in case e.g. "RHI_DIRECT3D12" was set as preprocessor definition
	// -> Example RHI names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	// -> In case the graphics driver supports it, the OpenGL ES 3 RHI can automatically also run on a desktop PC without an emulator (perfect for testing/debugging)
	mDefaultRhiName(Rhi::DEFAULT_RHI_NAME)
{
	// Sets of supported RHI implementations
	const std::array<std::string_view, 8> supportsAllRhi	   = {{"Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"}};
	const std::array<std::string_view, 6> onlyShaderModel4Plus = {{"Null", "Vulkan", "OpenGL", "Direct3D10", "Direct3D11", "Direct3D12"}};
	const std::array<std::string_view, 5> onlyShaderModel5Plus = {{"Null", "Vulkan", "OpenGL", "Direct3D11", "Direct3D12"}};

	// Basics
	addExample("Triangle",					&runRhiExample<Triangle>,						supportsAllRhi);
	addExample("IndirectBuffer",			&runRhiExample<IndirectBuffer>,					supportsAllRhi);
	addExample("Queries",					&runRhiExample<Queries>,						supportsAllRhi);
	addExample("VertexBuffer",				&runRhiExample<VertexBuffer>,					supportsAllRhi);
	addExample("Texture",					&runRhiExample<Texture>,						supportsAllRhi);
	addExample("CubeTexture",				&runRhiExample<CubeTexture>,					supportsAllRhi);
	addExample("RenderToTexture",			&runRhiExample<RenderToTexture>,				supportsAllRhi);
	addExample("MultipleRenderTargets",		&runRhiExample<MultipleRenderTargets>,			supportsAllRhi);
	#ifndef __ANDROID__
		addExample("MultipleSwapChains",	&runBasicExample<MultipleSwapChains>,			supportsAllRhi);
	#endif
	addExample("Instancing",				&runRhiExample<Instancing>,						supportsAllRhi);
	addExample("GeometryShader",			&runRhiExample<GeometryShader>,					onlyShaderModel4Plus);
	addExample("TessellationShader",		&runRhiExample<TessellationShader>,				onlyShaderModel5Plus);
	addExample("ComputeShader",				&runRhiExample<ComputeShader>,					onlyShaderModel5Plus);

	// Advanced
	addExample("Gpgpu",							&runBasicExample<Gpgpu>,					supportsAllRhi);
	addExample("IcosahedronTessellation",		&runRhiExample<IcosahedronTessellation>,	onlyShaderModel5Plus);
	#ifdef RENDERER_IMGUI
		addExample("InstancedCubes",			&runRenderExample<InstancedCubes>,			supportsAllRhi);
	#else
		addExample("InstancedCubes",			&runRhiExample<InstancedCubes>,				supportsAllRhi);
	#endif

	// Renderer
	#ifdef RENDERER
		#ifdef RENDERER_IMGUI
			addExample("ImGuiExampleSelector",	&runRenderExample<ImGuiExampleSelector>,	supportsAllRhi);
		#endif
		addExample("Mesh",						&runRenderExample<Mesh>,					supportsAllRhi);
		addExample("Compositor",				&runRenderExample<Compositor>,				supportsAllRhi);
		addExample("Scene",						&runRenderExample<Scene>,					supportsAllRhi);
		mDefaultExampleName = "ImGuiExampleSelector";
	#else
		mDefaultExampleName = "Triangle";
	#endif

	#ifdef RHI_NULL
		mAvailableRhis.insert("Null");
	#endif
	#ifdef RHI_VULKAN
		mAvailableRhis.insert("Vulkan");
	#endif
	#ifdef RHI_OPENGL
		mAvailableRhis.insert("OpenGL");
	#endif
	#ifdef RHI_OPENGLES3
		mAvailableRhis.insert("OpenGLES3");
	#endif
	#ifdef RHI_DIRECT3D9
		mAvailableRhis.insert("Direct3D9");
	#endif
	#ifdef RHI_DIRECT3D10
		mAvailableRhis.insert("Direct3D10");
	#endif
	#ifdef RHI_DIRECT3D11
		mAvailableRhis.insert("Direct3D11");
	#endif
	#ifdef RHI_DIRECT3D12
		mAvailableRhis.insert("Direct3D12");
	#endif
}

int ExampleRunner::run(const CommandLineArguments& commandLineArguments)
{
	if (!parseCommandLineArguments(commandLineArguments))
	{
		printUsage(mAvailableExamples, mAvailableRhis);
		return -1;
	}

	// Run example and switch as long between examples as asked to
	int result = 0;
	do
	{
		// Run current example
		result = runExample(mCurrentRhiName, mCurrentExampleName);
		if (0 == result && !mNextRhiName.empty() && !mNextExampleName.empty())
		{
			// Switch to next example
			mCurrentRhiName = mNextRhiName;
			mCurrentExampleName = mNextExampleName;
			mNextRhiName.clear();
			mNextExampleName.clear();
			result = 1;
		}
	} while (result);

	// Done
	return result;
}

void ExampleRunner::switchExample(const char* exampleName, const char* rhiName)
{
	ASSERT(nullptr != exampleName, "Invalid example name")
	mNextRhiName = (nullptr != rhiName) ? rhiName : mDefaultRhiName;
	mNextExampleName = exampleName;
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
bool ExampleRunner::parseCommandLineArguments(const CommandLineArguments& commandLineArguments)
{
	const uint32_t numberOfArguments = commandLineArguments.getCount();
	for (uint32_t argumentIndex = 0; argumentIndex < numberOfArguments; ++argumentIndex)
	{
		const std::string argument = commandLineArguments.getArgumentAtIndex(argumentIndex);
		if ("-r" != argument)
		{
			mCurrentExampleName = argument;
		}
		else if (argumentIndex + 1 < numberOfArguments)
		{
			++argumentIndex;
			mCurrentRhiName = commandLineArguments.getArgumentAtIndex(argumentIndex);
		}
		else
		{
			showError("Missing argument for parameter -r");

			// Error!
			return false;
		}
	}

	if (mCurrentRhiName.empty())
	{
		mCurrentRhiName = mDefaultRhiName;
	}

	// Done
	return true;
}

void ExampleRunner::printUsage(const AvailableExamples& availableExamples, const AvailableRhis& availableRhis)
{
	std::cout << "Usage: ./Examples <ExampleName> [-r <RhiName>]\n";

	// Available examples
	std::cout << "Available Examples:\n";
	for (const auto& pair : availableExamples)
	{
		std::cout << "\t" << pair.first << '\n';
	}

	// Available RHIs
	std::cout << "Available RHIs:\n";
	for (const std::string_view& rhiName : availableRhis)
	{
		std::cout << "\t" << rhiName << '\n';
	}
}

void ExampleRunner::showError(const std::string& errorMessage)
{
	std::cout << errorMessage << "\n";
}

int ExampleRunner::runExample(const std::string_view& rhiName, const std::string_view& exampleName)
{
	// Get selected RHI and selected example
	const AvailableRhis::iterator selectedRhi = mAvailableRhis.find(rhiName);
	const std::string_view& selectedExampleName = exampleName.empty() ? mDefaultExampleName : exampleName;
	const AvailableExamples::iterator selectedExample = mAvailableExamples.find(selectedExampleName);

	// Ensure the selected RHI is supported by the selected example
	ExampleToSupportedRhis::iterator supportedRhi = mExampleToSupportedRhis.find(selectedExampleName);
	bool rhiNotSupportedByExample = false;
	if (mExampleToSupportedRhis.end() != supportedRhi)
	{
		const SupportedRhis& supportedRhiList = supportedRhi->second;
		rhiNotSupportedByExample = std::find(supportedRhiList.begin(), supportedRhiList.end(), rhiName) == supportedRhiList.end();
	}
	if (mAvailableExamples.end() == selectedExample || mAvailableRhis.end() == selectedRhi || rhiNotSupportedByExample)
	{
		if (mAvailableExamples.end() == selectedExample)
		{
			showError("No or unknown example given");
		}
		if (mAvailableRhis.end() == selectedRhi)
		{
			showError("Unknown RHI: \"" + std::string(rhiName) + "\"");
		}
		if (rhiNotSupportedByExample)
		{
			showError("The example \"" + std::string(selectedExampleName) + "\" doesn't support RHI: \"" + std::string(rhiName) + "\"");
		}

		// Print usage
		printUsage(mAvailableExamples, mAvailableRhis);
		return 0;
	}
	else
	{
		// Run example
		return selectedExample->second(*this, std::string(rhiName).c_str(), exampleName);
	}
}
