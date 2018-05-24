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
#include "Framework/PlatformTypes.h"
#include "Framework/IApplicationFrontend.h"

#include <Renderer/Public/Renderer.h>

#include <SDL2/SDL.h>

#include <string>


//[-------------------------------------------------------]
//[ Definitions                                           ]
//[-------------------------------------------------------]
// TODO(co) "ExampleBase.h" and "IApplicationRenderer.h" use the same definitions
#ifdef RENDERER_ONLY_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_OPENGL
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_OPENGLES3
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_VULKAN
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_DIRECT3D9
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_DIRECT3D10
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_DIRECT3D11
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D12
#elif RENDERER_ONLY_DIRECT3D12
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
#endif


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class ExampleBase;
namespace Renderer
{
	class Context;
	class RendererInstance;
	class ISwapChain;
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Renderer application interface
*/
class IApplicationRenderer : public IApplicationFrontend, public Renderer::IRenderWindow
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
	*    Example renderer names: "Null", "OpenGL", "OpenGLES3", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*  @param[in] example
	*    Pointer to an example which should be used
	*/
	explicit IApplicationRenderer(const std::string& rendererName, ExampleBase* example);

	/**
	*  @brief
	*    Destructor
	*/
	virtual ~IApplicationRenderer();

	/**
	*  @brief
	*    Run the application
	*
	*  @return
	*    Program return code, 0 to indicate that no error has occurred
	*/
	int run();

	/**
	*  @brief
	*    Redraw request
	*/
	void redraw();


//[-------------------------------------------------------]
//[ Public virtual IApplicationRenderer methods           ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization();
	virtual void onDeinitialization();
	virtual void onUpdate();
	virtual void onResize(uint32_t width, uint32_t height);
	virtual void onToggleFullscreenState();
	virtual void onKeyDown(uint32_t key);
	virtual void onKeyUp(uint32_t key);
	virtual void onMouseButtonDown(uint32_t button);
	virtual void onMouseButtonUp(uint32_t button);
	virtual void onMouseMove(int x, int y);
	virtual void onMouseWheel(bool scrollUp);
	virtual void onDrawRequest();


//[-------------------------------------------------------]
//[ Public virtual IApplicationFrontend methods           ]
//[-------------------------------------------------------]
public:
	inline virtual Renderer::IRenderer* getRenderer() const override
	{
		return mRenderer;
	}

	inline virtual Renderer::IRenderTarget* getMainRenderTarget() const override
	{
		return mMainSwapChain;
	}


//[-------------------------------------------------------]
//[ Public virtual IRenderWindow methods                  ]
//[-------------------------------------------------------]
public:
	virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override;
	virtual void present() override;


//[-------------------------------------------------------]
//[ Protected virtual IApplicationRenderer methods        ]
//[-------------------------------------------------------]
protected:
	bool onInitializeApplication();


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
protected:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] rendererName
	*    Case sensitive ASCII name of the renderer to instance, if null pointer or unknown renderer no renderer will be used.
	*    Example renderer names: "Null", "OpenGL", "OpenGLES3", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*/
	explicit IApplicationRenderer(const char *rendererName);

	/**
	*  @brief
	*    Create the renderer instance when it not already exists
	*/
	void createRenderer();

	/**
	*  @brief
	*    Initialize the example, when not already done
	*/
	void initializeExample();

	/**
	*  @brief
	*    Deinitialize the example, when not already done
	*/
	void deinitializeExample();


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit IApplicationRenderer(const IApplicationRenderer& source) = delete;
	IApplicationRenderer& operator =(const IApplicationRenderer& source) = delete;

	/**
	*  @brief
	*    Create a renderer instance
	*
	*  @param[in] rendererName
	*    Case sensitive ASCII name of the renderer to instance, if null pointer nothing happens.
	*    Example renderer names: "Null", "OpenGL", "OpenGLES3", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*
	*  @return
	*    The created renderer instance, null pointer on error
	*/
	Renderer::IRenderer *createRendererInstance(const std::string& rendererName);

	bool processMessages();
	Renderer::Context* createRendererContext() const;
	void getWindowSize(uint32_t &width, uint32_t &height) const;
	Renderer::WindowInfo getWindowInfo(SDL_Window* sdlWindow);


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	std::string					mRendererName;		///< Case sensitive ASCII name of the renderer to instance
	std::string					mWindowTitle;
	Renderer::Context*			mRendererContext;	///< Renderer context, can be a null pointer
	Renderer::RendererInstance* mRendererInstance;	///< Renderer instance, can be a null pointer
	Renderer::IRenderer*		mRenderer;			///< Renderer instance, can be a null pointer, do not destroy the instance
	Renderer::ISwapChain*		mMainSwapChain;		///< Main swap chain instance, can be a null pointer, release the instance if you no longer need it
	Renderer::CommandBuffer		mCommandBuffer;		///< Command buffer
	ExampleBase*				mExample;
	SDL_Window	   *mMainWindow;		///< SDL2 window, can be a null pointer, destroy the instance in case you no longer need it
	SDL_GLContext	mOpenGLContext;
	
	uint32_t		mCurrentWindowWidth;
	uint32_t		mCurrentWindowHeight;


};
