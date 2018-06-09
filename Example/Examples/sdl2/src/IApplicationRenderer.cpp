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
#include "IApplicationRenderer.h"
#include "Framework/ExampleBase.h"

#include <Renderer/Public/StdLog.h>
#include <Renderer/Public/StdAssert.h>
#include <Renderer/Public/StdMemory.h>
#include <Renderer/Public/RendererInstance.h>

#include <SDL2/SDL_syswm.h>

#include <iostream>
#include <unordered_map>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		Renderer::StdLog	g_RendererLog;
		Renderer::StdAssert g_RendererAssert;
		Renderer::StdMemory g_RendererMemory;


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
IApplicationRenderer::IApplicationRenderer(const std::string& rendererName, ExampleBase* example) :
	mRendererName(rendererName),
	mWindowTitle("SDL2 " + mRendererName),
	mRendererContext(nullptr),
	mRendererInstance(nullptr),
	mRenderer(nullptr),
	mMainSwapChain(nullptr),
	mExample(example),
	mMainWindow(nullptr),
	mOpenGLContext(nullptr),
	mCurrentWindowWidth(640),
	mCurrentWindowHeight(480)
{
	if (nullptr != mExample)
	{
		mExample->setApplicationFrontend(this);
	}
}

IApplicationRenderer::~IApplicationRenderer()
{
	// Nothing here
}

int IApplicationRenderer::run()
{
	// Call application implementation initialization method
	onInitialization();

	// Main loop - Process OS messages (non-blocking) first
	while (!processMessages())
	{
		// Update the application logic
		onUpdate();

		// Redraw request
		redraw();
	}

	// Call application implementation de-initialization method
	onDeinitialization();

	// Done, no error
	return 0;
}

void IApplicationRenderer::redraw()
{
	if (nullptr != mMainWindow)
	{
		Uint32 windowFlags =  SDL_GetWindowFlags(mMainWindow);

		if (windowFlags & SDL_WINDOW_SHOWN)
			onDrawRequest();
	}
}


//[-------------------------------------------------------]
//[ Public virtual IApplicationRenderer methods           ]
//[-------------------------------------------------------]
void IApplicationRenderer::onInitialization()
{
	if(onInitializeApplication())
	{
		createRenderer();
		initializeExample();
	}
}

void IApplicationRenderer::onDeinitialization()
{
	deinitializeExample();

	// Delete the renderer instance
	if (nullptr != mMainSwapChain)
	{
		mMainSwapChain->releaseReference();
		mMainSwapChain = nullptr;
	}

	// Delete the renderer instance
	mRenderer = nullptr;

	if (nullptr != mRendererInstance)
	{
		mRendererInstance->destroyRenderer();
	}

	delete mRendererInstance;
	mRendererInstance = nullptr;
	delete mRendererContext;
	mRendererContext = nullptr;
	
	// Destroy the OS window instance, in case there's one
	if (nullptr != mMainWindow)
	{
		//Destroy window	
		SDL_DestroyWindow( mMainWindow );
		mMainWindow = nullptr;
	}

	//Quit SDL subsystems
	SDL_Quit();

}

void IApplicationRenderer::onUpdate()
{
	if (nullptr != mExample)
	{
		mExample->onUpdate();
	}
}

void IApplicationRenderer::onResize(uint32_t, uint32_t)
{
	// Is there a renderer instance?
	if (nullptr != mRenderer && nullptr != mMainSwapChain)
	{
		// Inform the swap chain that the size of the native window was changed
		// -> Required for Direct3D 9, Direct3D 10, Direct3D 11
		// -> Not required for OpenGL and OpenGL ES 3
		mMainSwapChain->resizeBuffers();
	}
}

void IApplicationRenderer::onToggleFullscreenState()
{
	// Is there a renderer instance?
	if (nullptr != mRenderer)
	{
		Uint32 windowFlags =  SDL_GetWindowFlags(mMainWindow);
		if (windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP)
			SDL_SetWindowFullscreen(mMainWindow, 0u);
		else
			SDL_SetWindowFullscreen(mMainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		
		// The size change is updated via windows size change event
	}
}

void IApplicationRenderer::onKeyDown(uint32_t key)
{
	if (nullptr != mExample)
	{
		mExample->onKeyDown(key);
	}
}

void IApplicationRenderer::onKeyUp(uint32_t key)
{
	if (nullptr != mExample)
	{
		mExample->onKeyUp(key);
	}
}

void IApplicationRenderer::onMouseButtonDown(uint32_t button)
{
	if (nullptr != mExample)
	{
		mExample->onMouseButtonDown(button);
	}
}

void IApplicationRenderer::onMouseButtonUp(uint32_t button)
{
	if (nullptr != mExample)
	{
		mExample->onMouseButtonUp(button);
	}
}

void IApplicationRenderer::onMouseMove(int x, int y)
{
	if (nullptr != mExample)
	{
		mExample->onMouseMove(x, y);
	}
}

void IApplicationRenderer::onMouseWheel(bool /*scrollUp*/)
{
	// Base does nothing
}

void IApplicationRenderer::onDrawRequest()
{
	if (nullptr != mExample && mExample->doesCompleteOwnDrawing())
	{
		// The example does the drawing completely on its own
		mExample->draw();
	}

	// Is there a renderer instance?
	else if (nullptr != mRenderer && nullptr != mMainSwapChain)
	{
		// Begin scene rendering
		// -> Required for Direct3D 9 and Direct3D 12
		// -> Not required for Direct3D 10, Direct3D 11, OpenGL and OpenGL ES 2
		if (mRenderer->beginScene())
		{
			{ // Scene rendering
				// Scoped debug event
				COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

				// Make the main swap chain to the current render target
				Renderer::Command::SetRenderTarget::create(mCommandBuffer, mMainSwapChain);

				{ // Since Direct3D 12 is command list based, the viewport and scissor rectangle
					// must be set in every draw call to work with all supported renderer APIs
					// Get the window size
					uint32_t width  = 1;
					uint32_t height = 1;

					getWindowSize(width, height);

					// Set the viewport and scissor rectangle
					Renderer::Command::SetViewportAndScissorRectangle::create(mCommandBuffer, 0, 0, width, height);
				}

				// Submit command buffer to the renderer backend
				mCommandBuffer.submitAndClear(*mRenderer);

				// Call the draw method
				if (nullptr != mExample)
				{
					mExample->draw();
				}
			}

			// Submit command buffer to the renderer backend
			mCommandBuffer.submitAndClear(*mRenderer);

			// End scene rendering
			// -> Required for Direct3D 9 and Direct3D 12
			// -> Not required for Direct3D 10, Direct3D 11, OpenGL and OpenGL ES 2
			mRenderer->endScene();
		}

		// Present the content of the current back buffer
		if (nullptr != mOpenGLContext)
		{
			SDL_GL_SwapWindow(mMainWindow);
		}
		else
		{
			mMainSwapChain->present();
		}
		//std::cout<<"SDL error?: " <<SDL_GetError()<<'\n';
	}
}


//[-------------------------------------------------------]
//[ Public virtual Renderer::IRenderWindow methods        ]
//[-------------------------------------------------------]
void IApplicationRenderer::getWidthAndHeight(uint32_t& width, uint32_t& height) const
{
	width = mCurrentWindowWidth;
	height = mCurrentWindowHeight;
}

void IApplicationRenderer::present()
{
	if (nullptr != mOpenGLContext)
	{
		SDL_GL_SwapWindow(mMainWindow);
	}
}


//[-------------------------------------------------------]
//[ Protected virtual IApplicationRenderer methods        ]
//[-------------------------------------------------------]
bool IApplicationRenderer::onInitializeApplication()
{
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		std::cout<<"SDL could not initialize! SDL Error: " <<SDL_GetError()<<'\n';
	}
	else
	{
		const bool isOpenGLRenderer = mRendererName == "OpenGLES3" || mRendererName == "OpenGL";
		Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
		if (isOpenGLRenderer)
		{
			windowFlags |= SDL_WINDOW_OPENGL;

			if (mRendererName == "OpenGLES3")
			{
				//Use OpenGL ES 3.0
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
			}
			else
			{
				SDL_GL_LoadLibrary(NULL); // Default OpenGL is fine.

				//Use OpenGL 4.1 core
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
				
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
				SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
				SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			}
		}

		mMainWindow = SDL_CreateWindow(mWindowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mCurrentWindowWidth, mCurrentWindowHeight, windowFlags);

		if (isOpenGLRenderer)
		{
			if (nullptr != mMainWindow)
			{
				mOpenGLContext = SDL_GL_CreateContext(mMainWindow);
			}

			if (nullptr != mMainWindow && nullptr != mOpenGLContext)
			{
				if( SDL_GL_SetSwapInterval( 1 ) < 0 )
				{
					std::cout<<"Warning: Unable to set VSync! SDL Error: "<< SDL_GetError()<<'\n';
				}

				// Make the opengl context current. This is needed so that the opengl renderer can be properly initialized
				SDL_GL_MakeCurrent(mMainWindow, mOpenGLContext);
				
				// Initialization went fine
				return true;
			}
		}
		else
		{
			return (nullptr != mMainWindow);
		}
	}

	return false;
}


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
IApplicationRenderer::IApplicationRenderer(const char *rendererName) :
	IApplicationRenderer(rendererName, nullptr)
{
	// Nothing here
}

void IApplicationRenderer::createRenderer()
{
	if (nullptr == mRenderer)
	{
		// Create the renderer instance
		mRenderer = createRendererInstance(mRendererName);

		if (nullptr != mRenderer)
		{
			// Create render pass using the preferred swap chain texture format
			const Renderer::Capabilities& capabilities = mRenderer->getCapabilities();
			Renderer::IRenderPass* renderPass = mRenderer->createRenderPass(1, &capabilities.preferredSwapChainColorTextureFormat, capabilities.preferredSwapChainDepthStencilTextureFormat);

			// Create a main swap chain instance
			mMainSwapChain = mRenderer->createSwapChain(*renderPass, getWindowInfo(mMainWindow), mRenderer->getContext().isUsingExternalContext());
			RENDERER_SET_RESOURCE_DEBUG_NAME(mMainSwapChain, "Main swap chain")
			mMainSwapChain->addReference();	// Internal renderer reference
		}
	}
}

void IApplicationRenderer::initializeExample()
{
	if (nullptr != mExample)
	{
		mExample->initialize();
	}
}

void IApplicationRenderer::deinitializeExample()
{
	if (nullptr != mExample)
	{
		mExample->deinitialize();
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
Renderer::IRenderer *IApplicationRenderer::createRendererInstance(const std::string& rendererName)
{
	// Is the given pointer valid?
	if (!rendererName.empty())
	{
		mRendererContext = createRendererContext();
		mRendererInstance = new Renderer::RendererInstance(rendererName.c_str(), *mRendererContext);
	}
	Renderer::IRenderer *renderer = (nullptr != mRendererInstance) ? mRendererInstance->getRenderer() : nullptr;

	// Is the renderer instance is properly initialized?
	if (nullptr != renderer && !renderer->isInitialized())
	{
		// We are not interested in not properly initialized renderer instances, so get rid of the broken thing
		renderer = nullptr;
		delete mRendererInstance;
		mRendererInstance = nullptr;
	}

	#ifndef RENDERER_DEBUG
		// By using
		//   "Renderer::IRenderer::isDebugEnabled()"
		// in here its possible to check whether or not your application is currently running
		// within a known debug/profile tool like e.g. Direct3D PIX (also works directly within VisualStudio
		// 2012 out-of-the-box). In case you want at least try to protect your asset, you might want to stop
		// the execution of your application when a debug/profile tool is used which can e.g. record your data.
		// Please be aware that this will only make it a little bit harder to debug and e.g. while doing so
		// reading out your asset data. Public articles like
		// "PIX: How to circumvent D3DPERF_SetOptions" at
		//   http://www.gamedev.net/blog/1323/entry-2250952-pix-how-to-circumvent-d3dperf-setoptions/
		// describe how to "hack around" this security measurement, so, don't rely on it. Those debug
		// methods work fine when using a Direct3D renderer implementation. OpenGL on the other hand
		// has no Direct3D PIX like functions or extensions, use for instance "gDEBugger" (http://www.gremedy.com/)
		// instead.
		if (nullptr != renderer && renderer->isDebugEnabled())
		{
			// We don't allow debugging in case debugging is disabled
			OUTPUT_DEBUG_STRING("Debugging with debug/profile tools like e.g. Direct3D PIX is disabled within this application")
			delete renderer;
			renderer = nullptr;
		}
	#endif

	// Done
	return renderer;
}

void PrintEvent(const SDL_Event * event)
{
    if (event->type == SDL_WINDOWEVENT) {
        switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
            SDL_Log("Window %d shown", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            SDL_Log("Window %d hidden", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            SDL_Log("Window %d exposed", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MOVED:
            SDL_Log("Window %d moved to %d,%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            SDL_Log("Window %d resized to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_Log("Window %d size changed to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            SDL_Log("Window %d minimized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            SDL_Log("Window %d maximized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_RESTORED:
            SDL_Log("Window %d restored", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_ENTER:
            SDL_Log("Mouse entered window %d",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            SDL_Log("Mouse left window %d", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            SDL_Log("Window %d gained keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            SDL_Log("Window %d lost keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            SDL_Log("Window %d closed", event->window.windowID);
            break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            SDL_Log("Window %d is offered a focus", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIT_TEST:
            SDL_Log("Window %d has a special hit test", event->window.windowID);
            break;
#endif
        default:
            SDL_Log("Window %d got unknown event %d",
                    event->window.windowID, event->window.event);
            break;
        }
    }
}

// Needed because the input handling of the scene example uses the ascii codes for checking if a WASD key is pressed or not
static std::unordered_map<uint32_t, uint32_t> SDL2KeySymToKeyMap {
												{SDLK_a, 'A'}, 
												{SDLK_w, 'W'}, 
												{SDLK_s, 'S'}, 
												{SDLK_d, 'D'}
};


bool IApplicationRenderer::processMessages()
{
	bool quit = false;
	SDL_Event e;
	while( SDL_PollEvent( &e ) != 0 )
	{
		PrintEvent(&e);
		//User requests quit
		if( e.type == SDL_QUIT )
		{
			quit = true;
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			switch (e.window.event) {
				case SDL_WINDOWEVENT_SHOWN:
					// When window gets shown update window resized information
					onResize(mCurrentWindowWidth, mCurrentWindowHeight);
				break;
				case SDL_WINDOWEVENT_EXPOSED:
					onDrawRequest();
				break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					mCurrentWindowHeight = e.window.data2;
					mCurrentWindowWidth = e.window.data1;
					onResize(mCurrentWindowWidth, mCurrentWindowHeight);
				break;
			}
		}
		else if (e.type == SDL_KEYDOWN)
		{
			if (!e.key.repeat && e.key.keysym.sym == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT))
			{
				onToggleFullscreenState();
			}
			else if (!e.key.repeat)
			{
				auto keyIterator = SDL2KeySymToKeyMap.find(e.key.keysym.sym);
				if (keyIterator != SDL2KeySymToKeyMap.end())
				{
					onKeyDown(keyIterator->second);
				}
			}
		}
		else if (e.type == SDL_KEYUP)
		{
			if (!e.key.repeat && e.key.keysym.sym == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT))
			{
				onToggleFullscreenState();
			}
			else if (!e.key.repeat)
			{
				auto keyIterator = SDL2KeySymToKeyMap.find(e.key.keysym.sym);
				if (keyIterator != SDL2KeySymToKeyMap.end())
				{
					onKeyUp(keyIterator->second);
				}
			}
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
			{
				onMouseButtonDown(0);
			}
			else if (e.button.button == SDL_BUTTON_RIGHT)
			{
				SDL_CaptureMouse(SDL_TRUE);
				onMouseButtonDown(1);
			}
		}
		else if (e.type == SDL_MOUSEBUTTONUP)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
			{
				onMouseButtonUp(0);
			}
			else if (e.button.button == SDL_BUTTON_RIGHT)
			{
				SDL_CaptureMouse(SDL_FALSE);
				onMouseButtonUp(1);
			}
		}
		else if (e.type == SDL_MOUSEMOTION)
		{
			onMouseMove(e.motion.x, e.motion.y);
		}
		else if (e.type == SDL_MOUSEWHEEL)
		{
			onMouseWheel(e.wheel.y < 0);
		}
	}
	return quit;
}

Renderer::Context* IApplicationRenderer::createRendererContext() const
{
	if (nullptr != mMainWindow)
	{
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);

		const bool isOpenGLRenderer = (mRendererName == "OpenGLES3" || mRendererName == "OpenGL");

		if (SDL_GetWindowWMInfo(mMainWindow,&info))
		{
			switch (info.subsystem)
			{
				#ifdef _WIN32
					case SDL_SYSWM_WINDOWS:
						return new Renderer::Context(::detail::g_RendererLog, ::detail::g_RendererAssert, ::detail::g_RendererMemory, info.info.win.window, isOpenGLRenderer);
				#endif
				#ifdef LINUX
					case SDL_SYSWM_X11:
						return new Renderer::X11Context(::detail::g_RendererLog, ::detail::g_RendererAssert, ::detail::g_RendererMemory, info.info.x11.display, info.info.x11.window, isOpenGLRenderer);

					case SDL_SYSWM_WAYLAND:
						return new Renderer::WaylandContext(::detail::g_RendererLog, ::detail::g_RendererAssert, ::detail::g_RendererMemory, info.info.wl.display, info.info.wl.surface, isOpenGLRenderer);
				#endif
			}
		}
	}
	return nullptr;
}

void IApplicationRenderer::getWindowSize(uint32_t& width, uint32_t& height) const
{
	// Is there a valid OS window?
	if (nullptr != mMainWindow)
	{
		width = mCurrentWindowWidth;
		height = mCurrentWindowHeight;
	}
	else
	{
		// There's no valid OS window, return known values
		width  = 0;
		height = 0;
	}
}

Renderer::WindowInfo IApplicationRenderer::getWindowInfo(SDL_Window* sdlWindow)
{
	Renderer::WindowInfo windowInfo{0, this, nullptr};

	if (nullptr != sdlWindow)
	{
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
	  
		if(SDL_GetWindowWMInfo(sdlWindow, &info))
		{
			switch(info.subsystem)
			{
		#ifdef _WIN32
				case SDL_SYSWM_WINDOWS:
					//return new Renderer::Context(Renderer::Context::ContextType::WINDOWS, ::detail::g_RendererLog, info.info.win.window, isOpenGLRenderer);
					windowInfo.nativeWindowHandle = info.info.win.window;
					break;
		#endif
		#ifdef LINUX
				case SDL_SYSWM_X11:
					windowInfo.nativeWindowHandle = info.info.x11.window;
					break;
				case SDL_SYSWM_WAYLAND:
					windowInfo.waylandSurface = info.info.wl.surface;
					break;
		#endif
			}
		}
	}

	return windowInfo;
}
