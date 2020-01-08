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
#include "Examples/Private/Framework/IApplicationRenderer.h"
#include "Examples/Private/Framework/ExampleBase.h"

#include <Renderer/Public/Context.h>
#include <Renderer/Public/RendererInstance.h>
#include <Renderer/Public/Asset/AssetManager.h>
#ifdef RENDERER_PROFILER
	#include <Renderer/Public/Core/RemoteryProfiler.h>
#endif
#ifdef __ANDROID__
	#include <Renderer/Public/Core/File/AndroidFileManager.h>

	#include <android_native_app_glue.h>
#else
	#include <Renderer/Public/Core/File/PhysicsFSFileManager.h>
#endif

#ifdef RENDERER_TOOLKIT
	#include <Renderer/Public/Core/File/DefaultFileManager.h>
	#include <Renderer/Public/Core/Platform/PlatformManager.h>

	#include <RendererToolkit/Public/RendererToolkitInstance.h>

	#include <exception>
#endif


//[-------------------------------------------------------]
//[ Public virtual IApplicationFrontend methods           ]
//[-------------------------------------------------------]
Renderer::IRenderer* IApplicationRenderer::getRenderer() const
{
	return (nullptr != mRendererInstance) ? mRendererInstance->getRenderer() : nullptr;
}

RendererToolkit::IRendererToolkit* IApplicationRenderer::getRendererToolkit()
{
	#ifdef RENDERER_TOOLKIT
		// Create the renderer toolkit instance, if required
		if (nullptr == mRendererToolkitInstance)
		{
			ASSERT(nullptr != mRendererInstance, "The renderer instance must be valid")
			const Renderer::IRenderer* renderer = mRendererInstance->getRenderer();
			Rhi::ILog& log = renderer->getRhi().getContext().getLog();
			Rhi::IAssert& assert = renderer->getRhi().getContext().getAssert();
			Rhi::IAllocator& allocator = renderer->getRhi().getContext().getAllocator();
			mRendererToolkitFileManager = new Renderer::DefaultFileManager(log, assert, allocator, mFileManager->getAbsoluteRootDirectory());
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
bool IApplicationRenderer::onInitialization()
{
	// Create the RHI instance
	createRhi();

	// Is there a valid RHI instance?
	Rhi::IRhi* rhi = getRhi();
	if (nullptr != rhi)
	{
		// Create the renderer instance
		#ifdef __ANDROID__
			struct android_app androidApp;	// TODO(co) Get Android app instance
			RHI_ASSERT(rhi->getContext(), nullptr != androidApp.activity->assetManager, "Invalid Android asset manager instance")
			mFileManager = new Renderer::AndroidFileManager(rhi->getContext().getLog(), rhi->getContext().getAssert(), rhi->getContext().getAllocator(), std_filesystem::canonical(std_filesystem::current_path() / "..").generic_string(), *androidApp.activity->assetManager);
		#else
			mFileManager = new Renderer::PhysicsFSFileManager(rhi->getContext().getLog(), std_filesystem::canonical(std_filesystem::current_path() / "..").generic_string());
		#endif
		#if defined(RENDERER_GRAPHICS_DEBUGGER) && defined(RENDERER_PROFILER)
			mProfiler = new Renderer::RemoteryProfiler(*rhi);
			mRendererContext = new Renderer::Context(*rhi, *mFileManager, *mGraphicsDebugger, *mProfiler);
		#elif defined RENDERER_GRAPHICS_DEBUGGER
			mRendererContext = new Renderer::Context(*rhi, *mFileManager, *mGraphicsDebugger);
		#elif defined RENDERER_PROFILER
			mProfiler = new Renderer::RemoteryProfiler(*rhi);
			mRendererContext = new Renderer::Context(*rhi, *mFileManager, *mProfiler);
		#else
			mRendererContext = new Renderer::Context(*rhi, *mFileManager);
		#endif
		mRendererInstance = new Renderer::RendererInstance(*mRendererContext);

		{
			Renderer::IRenderer* renderer = getRenderer();
			if (nullptr != renderer)
			{
				// Mount asset package
				bool mountAssetPackageResult = false;
				Renderer::AssetManager& assetManager = renderer->getAssetManager();
				bool rhiIsOpenGLES = (rhi->getNameId() == Rhi::NameId::OPENGLES3);
				if (rhiIsOpenGLES)
				{
					// Handy fallback for development: If the mobile data isn't there, use the PC data
					mountAssetPackageResult = (assetManager.mountAssetPackage("../DataMobile/Example/Content", "Example") != nullptr);
					if (!mountAssetPackageResult)
					{
						RHI_LOG(renderer->getContext(), COMPATIBILITY_WARNING, "The examples application failed to find \"../DataMobile/Example/Content\", using \"../DataPc/Example/Content\" as fallback")
						mountAssetPackageResult = (assetManager.mountAssetPackage("../DataPc/Example/Content", "Example") != nullptr);
						rhiIsOpenGLES = false;
					}
				}
				else
				{
					mountAssetPackageResult = (assetManager.mountAssetPackage("../DataPc/Example/Content", "Example") != nullptr);
				}
				if (!mountAssetPackageResult)
				{
					// Error!
					showUrgentMessage("Please start \"ExampleProjectCompiler\" before starting \"Examples\" for the first time");
					deinitialization();
					return false;
				}
				renderer->loadPipelineStateObjectCache();

				// Load renderer toolkit project to enable hot-reloading in case of asset changes
				#ifdef RENDERER_TOOLKIT
				{
					RendererToolkit::IRendererToolkit* rendererToolkit = getRendererToolkit();
					if (nullptr != rendererToolkit)
					{
						// The renderer toolkit project startup is done inside a background thread to not block the main thread
						mRendererToolkitProjectStartupThread = std::thread(&IApplicationRenderer::rendererToolkitProjectStartupThreadWorker, this, renderer, rendererToolkit, rhiIsOpenGLES);
					}
				}
				#endif
			}
		}
	}

	// Initialize the example now that the RHI instance should be created successfully
	mExampleBase.onInitialization();

	// Done
	return true;
}

void IApplicationRenderer::onDeinitialization()
{
	mExampleBase.onDeinitialization();
	deinitialization();
}

void IApplicationRenderer::onUpdate()
{
	Renderer::IRenderer* renderer = getRenderer();
	if (nullptr != renderer)
	{
		renderer->update();
	}

	// Call base implementation
	IApplicationRhi::onUpdate();
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void IApplicationRenderer::deinitialization()
{
	delete mRendererInstance;
	mRendererInstance = nullptr;
	delete mRendererContext;
	mRendererContext = nullptr;
	#ifdef RENDERER_PROFILER
		delete static_cast<Renderer::RemoteryProfiler*>(mProfiler);
	#endif
	#ifdef __ANDROID__
		delete static_cast<Renderer::AndroidFileManager*>(mFileManager);
	#else
		delete static_cast<Renderer::PhysicsFSFileManager*>(mFileManager);
	#endif
	mFileManager = nullptr;
	#ifdef RENDERER_TOOLKIT
		{
			mRendererToolkitProjectStartupThread.join();
			std::lock_guard<std::mutex> projectMutexLock(mProjectMutex);
			if (nullptr != mProject)
			{
				delete mProject;
				mProject = nullptr;
			}
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
			delete static_cast<Renderer::DefaultFileManager*>(mRendererToolkitFileManager);
			mRendererToolkitFileManager = nullptr;
		}
	#endif
	destroyRhi();
}

#ifdef RENDERER_TOOLKIT
	void IApplicationRenderer::rendererToolkitProjectStartupThreadWorker(Renderer::IRenderer* renderer, RendererToolkit::IRendererToolkit* rendererToolkit, bool rhiIsOpenGLES)
	{
		RENDERER_SET_CURRENT_THREAD_DEBUG_NAME("Project startup", "Renderer toolkit: Project startup")
		std::lock_guard<std::mutex> projectMutexLock(mProjectMutex);
		mProject = rendererToolkit->createProject();
		if (nullptr != mProject)
		{
			try
			{
				// Load project: Shippable executable binaries are inside e.g. "unrimp/Binary/Windows_x64_Shared" while development data source is located
				// at "unrimp/Example/DataSource/Example" and the resulting compiled/baked data ends up inside e.g. "unrimp/Binary/DataPc/Example"
				mProject->load("../../Example/DataSource/Example");
				mProject->startupAssetMonitor(*renderer, rhiIsOpenGLES ? "OpenGLES3_300" : "Direct3D11_50");
			}
			catch (const std::exception& e)
			{
				RHI_LOG(renderer->getContext(), CRITICAL, "Failed to load renderer toolkit project: %s", e.what())
			}
		}
	}
#endif
