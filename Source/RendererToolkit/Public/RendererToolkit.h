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


// Public comfort header putting everything within a single header


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Public/Renderer.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <vector>
	#include <string>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IRendererRuntime;
	typedef const char* AbsoluteDirectoryName;
}
namespace RendererToolkit
{
	class IProject;
	class IRendererToolkit;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Types                                                 ]
	//[-------------------------------------------------------]
	// RendererToolkit/IRendererToolkit.h
	class IRendererToolkit : public Renderer::RefCount<IRendererToolkit>
	{
	public:
		enum class State
		{
			IDLE,
			BUSY
		};
	public:
		virtual ~IRendererToolkit() override;
	public:
		[[nodiscard]] virtual IProject* createProject() = 0;
		[[nodiscard]] virtual State getState() const = 0;
	protected:
		IRendererToolkit();
		explicit IRendererToolkit(const IRendererToolkit& source) = delete;
		IRendererToolkit& operator =(const IRendererToolkit& source) = delete;
	};
	typedef Renderer::SmartRefCount<IRendererToolkit> IRendererToolkitPtr;

	// RendererToolkit/IProject.h
	class IProject : public Renderer::RefCount<IProject>
	{
	public:
		typedef std::vector<std::string> AbsoluteFilenames;
	public:
		virtual ~IProject() override;
	public:
		virtual void load(RendererRuntime::AbsoluteDirectoryName absoluteProjectDirectoryName) = 0;
		virtual void importAssets(const AbsoluteFilenames& absoluteSourceFilenames, const std::string& targetAssetPackageName, const std::string& targetDirectoryName = "Imported") = 0;
		virtual void compileAllAssets(const char* rendererTarget) = 0;
		virtual void startupAssetMonitor(RendererRuntime::IRendererRuntime& rendererRuntime, const char* rendererTarget) = 0;
		virtual void shutdownAssetMonitor() = 0;
	protected:
		IProject();
		explicit IProject(const IProject& source) = delete;
		IProject& operator =(const IProject& source) = delete;
	};
	typedef Renderer::SmartRefCount<IProject> IProjectPtr;


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
