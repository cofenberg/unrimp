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
#include "Framework/IApplicationRendererRuntime.h"
#include "Framework/ExampleBase.h"

#include <RendererRuntime/Context.h>
#include <RendererRuntime/Public/RendererRuntimeInstance.h>
#include <RendererRuntime/Asset/AssetManager.h>
#ifdef RENDERER_RUNTIME_PROFILER
	#include <RendererRuntime/Core/RemoteryProfiler.h>
#endif
#ifdef __ANDROID__
	#include <RendererRuntime/Core/File/AndroidFileManager.h>

	#include <android_native_app_glue.h>
#else
	#include <RendererRuntime/Core/File/PhysicsFSFileManager.h>
#endif

#ifdef RENDERER_TOOLKIT
	#include <RendererRuntime/Core/File/DefaultFileManager.h>

	#include <RendererToolkit/Public/RendererToolkitInstance.h>

	#include <exception>
#endif


//[-------------------------------------------------------]
//[ Public virtual IApplicationFrontend methods           ]
//[-------------------------------------------------------]
RendererRuntime::IRendererRuntime* IApplicationRendererRuntime::getRendererRuntime() const
{
	return (nullptr != mRendererRuntimeInstance) ? mRendererRuntimeInstance->getRendererRuntime() : nullptr;
}

RendererToolkit::IRendererToolkit* IApplicationRendererRuntime::getRendererToolkit()
{
	#ifdef RENDERER_TOOLKIT
		// Create the renderer toolkit instance, if required
		if (nullptr == mRendererToolkitInstance)
		{
			assert((nullptr != mRendererRuntimeInstance) && "The renderer runtime instance must be valid");
			const RendererRuntime::IRendererRuntime* rendererRuntime = mRendererRuntimeInstance->getRendererRuntime();
			Renderer::ILog& log = rendererRuntime->getRenderer().getContext().getLog();
			Renderer::IAssert& assert = rendererRuntime->getRenderer().getContext().getAssert();
			Renderer::IAllocator& allocator = rendererRuntime->getRenderer().getContext().getAllocator();
			mRendererToolkitFileManager = new RendererRuntime::DefaultFileManager(log, assert, allocator, mFileManager->getAbsoluteRootDirectory());
			mRendererToolkitContext = new RendererToolkit::Context(log, assert, allocator, *mRendererToolkitFileManager);
			mRendererToolkitInstance = new RendererToolkit::RendererToolkitInstance(*mRendererToolkitContext);
		}
		return (nullptr != mRendererToolkitInstance) ? mRendererToolkitInstance->getRendererToolkit() : nullptr;
	#else
		return nullptr;
	#endif
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void IApplicationRendererRuntime::onInitialization()
{
	// Create the renderer instance
	createRenderer();

	// Is there a valid renderer instance?
	Renderer::IRenderer* renderer = getRenderer();
	if (nullptr != renderer)
	{
		// Create the renderer runtime instance
		#ifdef __ANDROID__
			struct android_app androidApp;	// TODO(co) Get Android app instance
			assert((nullptr != androidApp.activity->assetManager) && "Invalid Android asset manager instance");
			mFileManager = new RendererRuntime::AndroidFileManager(renderer->getContext().getLog(), renderer->getContext().getAssert(), renderer->getContext().getAllocator(), (std_filesystem::canonical(std_filesystem::current_path()) / "..").generic_string(), *androidApp.activity->assetManager);
		#else
			mFileManager = new RendererRuntime::PhysicsFSFileManager(renderer->getContext().getLog(), (std_filesystem::canonical(std_filesystem::current_path()) / "..").generic_string());
		#endif
		#ifdef RENDERER_RUNTIME_PROFILER
			mProfiler = new RendererRuntime::RemoteryProfiler(*renderer);
			mRendererRuntimeContext = new RendererRuntime::Context(*renderer, *mFileManager, *mProfiler);
		#else
			mRendererRuntimeContext = new RendererRuntime::Context(*renderer, *mFileManager);
		#endif
		mRendererRuntimeInstance = new RendererRuntime::RendererRuntimeInstance(*mRendererRuntimeContext);

		{
			RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
			if (nullptr != rendererRuntime)
			{
				// Add used asset package
				bool rendererIsOpenGLES = (renderer->getNameId() == Renderer::NameId::OPENGLES3);
				if (rendererIsOpenGLES)
				{
					// Handy fallback for development: If the mobile data isn't there, use the PC data
					if (mFileManager->doesFileExist("../DataMobile/Example/Content"))
					{
						rendererRuntime->getAssetManager().mountAssetPackage("../DataMobile/Example/Content", "Example");
					}
					else
					{
						RENDERER_LOG(rendererRuntime->getContext(), COMPATIBILITY_WARNING, "The examples application failed to find \"../DataMobile/Example/Content\", using \"../DataPc/Example/Content\" as fallback")
						rendererRuntime->getAssetManager().mountAssetPackage("../DataPc/Example/Content", "Example");
						rendererIsOpenGLES = false;
					}
				}
				else
				{
					rendererRuntime->getAssetManager().mountAssetPackage("../DataPc/Example/Content", "Example");
				}
				rendererRuntime->loadPipelineStateObjectCache();

				// Load renderer toolkit project to enable hot-reloading in case of asset changes
				#ifdef RENDERER_TOOLKIT
				{
					RendererToolkit::IRendererToolkit* rendererToolkit = getRendererToolkit();
					if (nullptr != rendererToolkit)
					{
						mProject = rendererToolkit->createProject();
						if (nullptr != mProject)
						{
							try
							{
								mProject->load("../DataSource/Example");
								mProject->startupAssetMonitor(*rendererRuntime, rendererIsOpenGLES ? "OpenGLES3_300" : "Direct3D11_50");
							}
							catch (const std::exception& e)
							{
								RENDERER_LOG(renderer->getContext(), CRITICAL, "Failed to load renderer toolkit project: %s", e.what())
							}
						}
					}
				}
				#endif
			}
		}
	}

	// Initialize the example now that the renderer instance should be created successfully
	mExampleBase.onInitialization();
}

void IApplicationRendererRuntime::onDeinitialization()
{
	// Deinitialize example before we tear down any dependencies
	mExampleBase.onDeinitialization();

	// Delete the renderer runtime instance
	delete mRendererRuntimeInstance;
	mRendererRuntimeInstance = nullptr;
	delete mRendererRuntimeContext;
	mRendererRuntimeContext = nullptr;
	#ifdef RENDERER_RUNTIME_PROFILER
		delete static_cast<RendererRuntime::RemoteryProfiler*>(mProfiler);
	#endif
	#ifdef __ANDROID__
		delete static_cast<RendererRuntime::AndroidFileManager*>(mFileManager);
	#else
		delete static_cast<RendererRuntime::PhysicsFSFileManager*>(mFileManager);
	#endif
	mFileManager = nullptr;
	#ifdef RENDERER_TOOLKIT
		if (nullptr != mProject)
		{
			delete mProject;
			mProject = nullptr;
		}
		if (nullptr != mRendererToolkitInstance)
		{
			delete mRendererToolkitInstance;
			mRendererToolkitInstance = nullptr;
		}
		if (nullptr != mRendererToolkitContext)
		{
			delete mRendererToolkitContext;
			mRendererToolkitContext = nullptr;
		}
		if (nullptr != mRendererToolkitFileManager)
		{
			delete static_cast<RendererRuntime::DefaultFileManager*>(mRendererToolkitFileManager);
			mRendererToolkitFileManager = nullptr;
		}
	#endif

	// Destroy renderer
	destroyRenderer();
}

void IApplicationRendererRuntime::onUpdate()
{
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		rendererRuntime->update();
	}

	// Call base implementation
	IApplicationRenderer::onUpdate();
}
