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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Framework/IApplicationRenderer.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class Context;
	class IFileManager;
	class IRendererRuntime;
	class RendererRuntimeInstance;
	#ifdef RENDERER_RUNTIME_PROFILER
		class IProfiler;
	#endif
}
namespace RendererToolkit
{
	class IRendererToolkit;
	#ifdef RENDERER_TOOLKIT
		class Context;
		class IProject;
		class RendererToolkitInstance;
	#endif
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Renderer runtime application interface
*/
class IApplicationRendererRuntime final : public IApplicationRenderer
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] rendererName
	*    Case sensitive ASCII name of the renderer to instance, if null pointer or unknown renderer no renderer will be used.
	*    Example renderer names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*  @param[in] exampleBase
	*    Reference to an example which should be used
	*/
	inline IApplicationRendererRuntime(const char* rendererName, ExampleBase& exampleBase) :
		IApplicationRenderer(rendererName, exampleBase),
		mFileManager(nullptr),
		#ifdef RENDERER_RUNTIME_PROFILER
			mProfiler(nullptr),
		#endif
		mRendererRuntimeContext(nullptr),
		mRendererRuntimeInstance(nullptr)
		#ifdef RENDERER_TOOLKIT
			, mRendererToolkitFileManager(nullptr)
			, mRendererToolkitContext(nullptr)
			, mRendererToolkitInstance(nullptr)
			, mProject(nullptr)
		#endif
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~IApplicationRendererRuntime() override
	{
		// Nothing here

		// "mFileManager", "mProfiler" and "mRendererRuntimeInstance" is destroyed within "onDeinitialization()"
	}


//[-------------------------------------------------------]
//[ Public virtual IApplicationFrontend methods           ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Return the renderer runtime instance
	*
	*  @return
	*    The renderer runtime instance, can be a null pointer
	*/
	[[nodiscard]] virtual RendererRuntime::IRendererRuntime* getRendererRuntime() const override;

	/**
	*  @brief
	*    Return the renderer toolkit instance
	*
	*  @return
	*    The renderer toolkit instance, can be a null pointer
	*
	*  @remarks
	*    During runtime, the renderer toolkit can optionally be used to enable asset hot-reloading. Meaning,
	*    as soon as an source asset gets changed, the asset is recompiled in a background thread and the compiled
	*    runtime-ready asset is reloaded. One can see the change in realtime without the need to restart the application.
	*
	*    This feature links during runtime the renderer toolkit as soon as this method is accessed the first time. If
	*    the renderer toolkit shared library is not there, this method will return a null pointer. This is a developer-feature
	*    and as such, it's not available in static builds which are meant for the end-user who e.g. just want to "play the game".
	*/
	[[nodiscard]] virtual RendererToolkit::IRendererToolkit* getRendererToolkit() override;


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onUpdate() override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit IApplicationRendererRuntime(const IApplicationRendererRuntime& source) = delete;
	IApplicationRendererRuntime& operator =(const IApplicationRendererRuntime& source) = delete;


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	RendererRuntime::IFileManager*			  mFileManager;				///< File manager instance, can be a null pointer
	#ifdef RENDERER_RUNTIME_PROFILER
		RendererRuntime::IProfiler*			  mProfiler;				///< Profiler instance, can be a null pointer
	#endif
	RendererRuntime::Context*				  mRendererRuntimeContext;	///< Renderer runtime context instance, can be a null pointer
	RendererRuntime::RendererRuntimeInstance* mRendererRuntimeInstance;	///< Renderer runtime instance, can be a null pointer
	#ifdef RENDERER_TOOLKIT
		RendererRuntime::IFileManager*			  mRendererToolkitFileManager;	///< Renderer toolkit file manager instance to avoid intermixing virtual filenames with the renderer runtime, can be a null pointer
		RendererToolkit::Context*				  mRendererToolkitContext;		///< Renderer toolkit context instance, can be a null pointer
		RendererToolkit::RendererToolkitInstance* mRendererToolkitInstance;		///< Renderer toolkit instance, can be a null pointer
		RendererToolkit::IProject*				  mProject;
	#endif


};
