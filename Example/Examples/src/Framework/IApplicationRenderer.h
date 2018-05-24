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
#include "Framework/IApplication.h"
#include "Framework/IApplicationFrontend.h"

#include <Renderer/Renderer.h>


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
#elifdef RENDERER_ONLY_OPENGL
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elifdef RENDERER_ONLY_OPENGLES3
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elifdef RENDERER_ONLY_VULKAN
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elifdef RENDERER_ONLY_DIRECT3D9
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elifdef RENDERER_ONLY_DIRECT3D10
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elifdef RENDERER_ONLY_DIRECT3D11
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D12
#elifdef RENDERER_ONLY_DIRECT3D12
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
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Renderer application interface
*/
class IApplicationRenderer : public IApplication, public IApplicationFrontend
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
	*  @param[in] exampleBase
	*    Pointer to an example which should be used
	*/
	IApplicationRenderer(const char* rendererName, ExampleBase* exampleBase);

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~IApplicationRenderer() override
	{
		// Nothing here
	}


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
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onUpdate() override;
	virtual void onResize() override;
	virtual void onToggleFullscreenState() override;
	virtual void onDrawRequest() override;


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
	inline explicit IApplicationRenderer(const char* rendererName) :
		IApplicationRenderer(rendererName, nullptr)
	{
		// Nothing here
	}

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
	Renderer::IRenderer* createRendererInstance(const char* rendererName);


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	char						mRendererName[32];	///< Case sensitive ASCII name of the renderer to instance
	Renderer::Context*			mRendererContext;	///< Renderer context, can be a null pointer
	Renderer::RendererInstance* mRendererInstance;	///< Renderer instance, can be a null pointer
	Renderer::IRenderer*		mRenderer;			///< Renderer instance, can be a null pointer, do not destroy the instance
	Renderer::ISwapChain*		mMainSwapChain;		///< Main swap chain instance, can be a null pointer, release the instance if you no longer need it
	Renderer::CommandBuffer		mCommandBuffer;		///< Command buffer
	ExampleBase*				mExampleBase;


};
