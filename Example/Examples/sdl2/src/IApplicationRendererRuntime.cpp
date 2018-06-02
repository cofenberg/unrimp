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
#include <exception>


//[-------------------------------------------------------]
//[ Preprocessor                                          ]
//[-------------------------------------------------------]
#ifndef RENDERER_NO_RUNTIME


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "IApplicationRendererRuntime.h"

#ifdef RENDERER_TOOLKIT
	#include <RendererToolkit/Public/RendererToolkitInstance.h>
#endif

#include <RendererRuntime/Public/RendererRuntimeInstance.h>
#include <RendererRuntime/Core/File/StdFileManager.h>
#include <RendererRuntime/Asset/AssetManager.h>
#include <RendererRuntime/DebugGui/Detail/DebugGuiManagerLinux.h>


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
IApplicationRendererRuntime::IApplicationRendererRuntime(const std::string& rendererName, ExampleBase* exampleBase) :
	IApplicationRenderer(rendererName, exampleBase),
	mFileManager(nullptr),
	mRendererRuntimeInstance(nullptr)
	#ifdef RENDERER_TOOLKIT
		, mRendererToolkitInstance(nullptr)
		, mProject(nullptr)
	#endif
{
	// Nothing here
}

IApplicationRendererRuntime::~IApplicationRendererRuntime()
{
	// Nothing here

	// "mFileManager" and "mRendererRuntimeInstance" is destroyed within "onDeinitialization()"
}


//[-------------------------------------------------------]
//[ Public virtual IApplicationFrontend methods           ]
//[-------------------------------------------------------]
RendererRuntime::IRendererRuntime *IApplicationRendererRuntime::getRendererRuntime() const
{
	return (nullptr != mRendererRuntimeInstance) ? mRendererRuntimeInstance->getRendererRuntime() : nullptr;
}

RendererToolkit::IRendererToolkit *IApplicationRendererRuntime::getRendererToolkit()
{
	#ifdef RENDERER_TOOLKIT
		// Create the renderer toolkit instance, if required
		if (nullptr == mRendererToolkitInstance)
		{
			assert(nullptr != mRendererRuntimeInstance && "The renderer runtime instance must be valid");
			mRendererToolkitInstance = new RendererToolkit::RendererToolkitInstance(mRendererRuntimeInstance->getRendererRuntime()->getFileManager());
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
	// Don't call the base, this would break examples which depends on renderer runtime instance

	if(onInitializeApplication())
	{
		// Create the renderer instance
		createRenderer();

		// Is there a valid renderer instance?
		Renderer::IRenderer *renderer = getRenderer();
		if (nullptr != renderer)
		{
			// Create the renderer runtime instance
			mFileManager = new RendererRuntime::StdFileManager(renderer->getContext().getLog());
			mRendererRuntimeContext = new RendererRuntime::Context(*renderer, *mFileManager);
			mRendererRuntimeInstance = new RendererRuntime::RendererRuntimeInstance(*mRendererRuntimeContext);

			{
				RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
				if (nullptr != rendererRuntime)
				{
					// TODO(co) Under construction: Will probably become "mount asset package"
					// Add used asset package
					const bool rendererIsOpenGLES = (0 == strcmp(renderer->getName(), "OpenGLES3"));
					if (rendererIsOpenGLES)
					{
						rendererRuntime->getAssetManager().addAssetPackageByFilename("Example/Content", "../DataMobile/Content/AssetPackage.assets");
					}
					else
					{
						rendererRuntime->getAssetManager().addAssetPackageByFilename("Example/Content", "../DataPc/Content/AssetPackage.assets");
					}
					rendererRuntime->loadPipelineStateObjectCache();

					#ifdef RENDERER_TOOLKIT
					{
						// TODO(co) First asset hot-reloading test
						RendererToolkit::IRendererToolkit* rendererToolkit = getRendererToolkit();
						if (nullptr != rendererToolkit)
						{
							mProject = rendererToolkit->createProject();
							if (nullptr != mProject)
							{
								try
								{
									mProject->loadByFilename("../DataSource/Example.project");
									if (rendererIsOpenGLES)
									{
										mProject->startupAssetMonitor(*rendererRuntime, "OpenGLES3_300");
									}
									else
									{
										mProject->startupAssetMonitor(*rendererRuntime, "Direct3D11_50");
									}
								}
								catch (const std::exception& e)
								{
									const char* text = e.what();
									text = text;
								}
							}
						}
					}
					#endif
				}
			}
		}

		// Initialize the example now that the renderer instance should be created successfully
		initializeExample();
	}
}

void IApplicationRendererRuntime::onDeinitialization()
{
	// Deinitinitialize example before we tear down any dependencies
	// -> The base class calls this too but this is safe to do because the deinitialization is only done when the example wasn't already deinitialized
	deinitializeExample();

	// Delete the renderer runtime instance
	delete mRendererRuntimeInstance;
	mRendererRuntimeInstance = nullptr;
	delete static_cast<RendererRuntime::StdFileManager*>(mFileManager);
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
	#endif

	// Call the base implementation
	IApplicationRenderer::onDeinitialization();
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

void IApplicationRendererRuntime::onResize(uint32_t width, uint32_t height)
{
	// Call base implementation
	IApplicationRenderer::onResize(width, height);

	const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		RendererRuntime::DebugGuiManagerLinux& debugGuiLinux = static_cast<RendererRuntime::DebugGuiManagerLinux&>(rendererRuntime->getDebugGuiManager());
		debugGuiLinux.onWindowResize(width, height);
	}
}

void IApplicationRendererRuntime::onKeyDown(uint32_t key)
{
	IApplicationRenderer::onKeyDown(key);
}

void IApplicationRendererRuntime::onKeyUp(uint32_t key)
{
	IApplicationRenderer::onKeyUp(key);
}

void IApplicationRendererRuntime::onMouseButtonDown(uint32_t button)
{
	IApplicationRenderer::onMouseButtonDown(button);

	const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		RendererRuntime::DebugGuiManagerLinux& debugGuiLinux = static_cast<RendererRuntime::DebugGuiManagerLinux&>(rendererRuntime->getDebugGuiManager());

		// The button index is zero based (0 = left mouse button) RendererRuntime::DebugGuiManagerLinux currently expect the mouse button be 1 based (1 = left mouse button) compensate it here
		debugGuiLinux.onMouseButtonInput(button+1, true);
	}
}

void IApplicationRendererRuntime::onMouseButtonUp(uint32_t button)
{
	IApplicationRenderer::onMouseButtonUp(button);

	const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		RendererRuntime::DebugGuiManagerLinux& debugGuiLinux = static_cast<RendererRuntime::DebugGuiManagerLinux&>(rendererRuntime->getDebugGuiManager());

		// The button index is zero based (0 = left mouse button) RendererRuntime::DebugGuiManagerLinux currently expect the mouse button be 1 based (1 = left mouse button) compensate it here
		debugGuiLinux.onMouseButtonInput(button+1, false);
	}
}

void IApplicationRendererRuntime::onMouseMove(int x, int y)
{
	IApplicationRenderer::onMouseMove(x, y);

	const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		RendererRuntime::DebugGuiManagerLinux& debugGuiLinux = static_cast<RendererRuntime::DebugGuiManagerLinux&>(rendererRuntime->getDebugGuiManager());
		debugGuiLinux.onMouseMoveInput(x, y);
	}
// #ifndef RENDERER_NO_RUNTIME
// 
// 		// TODO(co) Evil cast ahead. Maybe simplify the example application framework? After all, it's just an example framework for Unrimp and nothing too generic.
// 		const IApplicationRendererRuntime* applicationRendererRuntime = dynamic_cast<IApplicationRendererRuntime*>(this);
// 		if (nullptr != applicationRendererRuntime)
// 		{
// 			const RendererRuntime::IRendererRuntime* rendererRuntime = applicationRendererRuntime->getRendererRuntime();
// 			if (nullptr != rendererRuntime)
// 			{
// 				RendererRuntime::DebugGuiManagerLinux& debugGuiLinux = static_cast<RendererRuntime::DebugGuiManagerLinux&>(rendererRuntime->getDebugGuiManager());
// 				switch(event.type)
// 				{
// 					
// 					case KeyPress:
// 					case KeyRelease:
// 					{
// 						const int buffer_size = 2;
// 						char buffer[buffer_size + 1];
// 						KeySym keySym;
// 						int count = XLookupString(&event.xkey, buffer, buffer_size, &keySym, nullptr);
// 						buffer[count] = 0;
// 
// 						debugGuiLinux.onKeyInput(keySym, buffer[0], event.type == KeyPress);
// 						break;
// 					}
// 			}
// 		}
// #endif
}

void IApplicationRendererRuntime::onMouseWheel(bool scrollUp)
{
	IApplicationRenderer::onMouseWheel(scrollUp);

	const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		RendererRuntime::DebugGuiManagerLinux& debugGuiLinux = static_cast<RendererRuntime::DebugGuiManagerLinux&>(rendererRuntime->getDebugGuiManager());
		debugGuiLinux.onMouseWheelInput(scrollUp);
	}
}


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
IApplicationRendererRuntime::IApplicationRendererRuntime(const std::string& rendererName) :
	IApplicationRendererRuntime(rendererName, nullptr)
{
	// Nothing here
}


//[-------------------------------------------------------]
//[ Preprocessor                                          ]
//[-------------------------------------------------------]
#endif // RENDERER_NO_RUNTIME
