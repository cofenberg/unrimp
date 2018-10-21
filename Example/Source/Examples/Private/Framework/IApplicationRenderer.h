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
#include "Examples/Private/Framework/IApplication.h"
#include "Examples/Private/Framework/IApplicationFrontend.h"

#include <Renderer/Public/Renderer.h>


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
	*    Example renderer names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*  @param[in] exampleBase
	*    Reference to an example which should be used
	*/
	IApplicationRenderer(const char* rendererName, ExampleBase& exampleBase);

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
	virtual void switchExample(const char* exampleName, const char* rendererName = nullptr) override;

	inline virtual void exit() override
	{
		IApplication::exit();
	}

	[[nodiscard]] inline virtual Renderer::IRenderer* getRenderer() const override
	{
		return mRenderer;
	}

	[[nodiscard]] inline virtual Renderer::IRenderTarget* getMainRenderTarget() const override
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
	virtual void onEscapeKey() override;


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
protected:
	/**
	*  @brief
	*    Create the renderer instance
	*/
	void createRenderer();

	/**
	*  @brief
	*    Destroy the renderer instance
	*/
	void destroyRenderer();


//[-------------------------------------------------------]
//[ Protected data                                        ]
//[-------------------------------------------------------]
protected:
	ExampleBase& mExampleBase;


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
	*    Example renderer names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*
	*  @return
	*    The created renderer instance, null pointer on error
	*/
	[[nodiscard]] Renderer::IRenderer* createRendererInstance(const char* rendererName);


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


};
