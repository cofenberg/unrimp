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
#include "Examples/Private/Basics/MeshShader/MeshShader.h"
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

// "ini.h"-library implementation in here since the tiny external library is used by multiple examples
#define INI_IMPLEMENTATION
#define INI_MALLOC(ctx, size) (static_cast<Rhi::IAllocator*>(ctx)->reallocate(nullptr, 0, size, 1))
#define INI_FREE(ctx, ptr) (static_cast<Rhi::IAllocator*>(ctx)->reallocate(ptr, 0, 0, 1))
#include <ini/ini.h>

#ifdef _WIN32
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		// Set Windows version to Windows Vista (0x0600), we don't support Windows XP (0x0501)
		#ifdef WINVER
			#undef WINVER
		#endif
		#define WINVER			0x0600
		#ifdef _WIN32_WINNT
			#undef _WIN32_WINNT
		#endif
		#define _WIN32_WINNT	0x0600

		// Exclude some stuff from "windows.h" to speed up compilation a bit
		#define WIN32_LEAN_AND_MEAN
		#define NOGDICAPMASKS
		#define NOMENUS
		#define NOICONS
		#define NOKEYSTATES
		#define NOSYSCOMMANDS
		#define NORASTEROPS
		#define OEMRESOURCE
		#define NOATOM
		#define NOMEMMGR
		#define NOMETAFILE
		#define NOOPENFILE
		#define NOSCROLL
		#define NOSERVICE
		#define NOSOUND
		#define NOWH
		#define NOCOMM
		#define NOKANJI
		#define NOHELP
		#define NOPROFILER
		#define NODEFERWINDOWPOS
		#define NOMCX
		#define NOCRYPT

		// Disable warnings in external headers, we can't fix them
		__pragma(warning(push))
			__pragma(warning(disable: 5039))	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception. (compiling source file src\CommandLineArguments.cpp)
			#include <Windows.h>
		__pragma(warning(pop))

		// Get rid of some nasty OS macros
		#undef min
		#undef max
	PRAGMA_WARNING_POP
#elif __ANDROID__
	#include <android/log.h>
#else
	#error "Unsupported platform"
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
	const std::array<std::string_view, 5> meshShaderNeeded	   = {{"Null", "Vulkan", "OpenGL", "Direct3D12"}};

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
		addExample("MultipleSwapChains",	&runRhiExample<MultipleSwapChains>,				supportsAllRhi);
	#endif
	addExample("Instancing",				&runRhiExample<Instancing>,						supportsAllRhi);
	addExample("GeometryShader",			&runRhiExample<GeometryShader>,					onlyShaderModel4Plus);
	addExample("TessellationShader",		&runRhiExample<TessellationShader>,				onlyShaderModel5Plus);
	addExample("ComputeShader",				&runRhiExample<ComputeShader>,					onlyShaderModel5Plus);
	addExample("MeshShader",				&runRhiExample<MeshShader>,						meshShaderNeeded);

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
			showMessage("Missing argument for parameter -r", true);

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
	showMessage("Usage: ./Examples <ExampleName> [-r <RhiName>]");

	// Available examples
	showMessage("Available Examples:");
	for (const auto& pair : availableExamples)
	{
		showMessage("\t" + std::string(pair.first));
	}

	// Available RHIs
	showMessage("Available RHIs:");
	for (const std::string_view& rhiName : availableRhis)
	{
		showMessage("\t" + std::string(rhiName));
	}
}

void ExampleRunner::showMessage(const std::string& message, bool isError)
{
	std::string fullMessage;
	if (isError)
	{
		fullMessage += "Error: ";
	}	
	fullMessage += message;
	fullMessage += '\n';

	// Platform specific handling
	#ifdef _WIN32
	{
		// Convert UTF-8 string to UTF-16
		std::wstring utf16Line;
		utf16Line.resize(static_cast<std::wstring::size_type>(::MultiByteToWideChar(CP_UTF8, 0, fullMessage.c_str(), static_cast<int>(fullMessage.size()), nullptr , 0)));
		::MultiByteToWideChar(CP_UTF8, 0, fullMessage.c_str(), static_cast<int>(fullMessage.size()), utf16Line.data(), static_cast<int>(utf16Line.size()));

		// Write into standard output stream
		if (isError)
		{
			std::wcerr << utf16Line.c_str();
		}
		else
		{
			std::wcout << utf16Line.c_str();
		}

		// On Microsoft Windows, ensure the output can be seen inside the Visual Studio output window as well
		::OutputDebugStringW(utf16Line.c_str());
		if (isError && ::IsDebuggerPresent())
		{
			__debugbreak();
		}
	}
	#elif __ANDROID__
		__android_log_write(isError ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, "Unrimp", fullMessage.c_str());	// TODO(co) Might make sense to make the app-name customizable
	#elif LINUX
		// Write into standard output stream
		if (isError)
		{
			std::cerr << fullMessage.c_str();
		}
		else
		{
			std::cout << fullMessage.c_str();
		}
	#else
		#error "Unsupported platform"
	#endif
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
			showMessage("No or unknown example given", true);
		}
		if (mAvailableRhis.end() == selectedRhi)
		{
			showMessage("Unknown RHI: \"" + std::string(rhiName) + "\"", true);
		}
		if (rhiNotSupportedByExample)
		{
			showMessage("The example \"" + std::string(selectedExampleName) + "\" doesn't support RHI: \"" + std::string(rhiName) + "\"", true);
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
