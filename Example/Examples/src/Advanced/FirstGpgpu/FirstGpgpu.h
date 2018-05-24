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
#include <Renderer/Renderer.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class RendererInstance;
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    A first example showing how to use do General Purpose Computation on Graphics Processing Unit (GPGPU) by using the renderer interface and shaders without having any output window
*
*  @remarks
*    Demonstrates:
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - 2D texture
*    - Sampler state object
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Pipeline state object (PSO)
*    - Framebuffer object (FBO) used for render to texture
*    - General Purpose Computation on Graphics Processing Unit (GPGPU) by using the renderer interface and shaders without having any output window
*/
class FirstGpgpu final
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
	*/
	explicit FirstGpgpu(const char* rendererName);

	/**
	*  @brief
	*    Destructor
	*/
	inline ~FirstGpgpu()
	{
		// The resources are released within "onDeinitialization()"
		// Nothing here
	}

	/**
	*  @brief
	*    Run the application
	*
	*  @return
	*    Program return code, 0 to indicate that no error has occurred
	*/
	int run();


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	/**
	*  @brief
	*    Called on application initialization
	*
	*  @note
	*    - When this method is called it's ensured that the renderer instance "mRenderer" is valid
	*/
	void onInitialization();

	/**
	*  @brief
	*    Called on application de-initialization
	*
	*  @note
	*    - When this method is called it's ensured that the renderer instance "mRenderer" is valid
	*/
	void onDeinitialization();

	/**
	*  @brief
	*    Fill command buffer content generation
	*
	*  @note
	*    - When this method is called it's ensured that the renderer instance "mRenderer" is valid
	*/
	void fillCommandBufferContentGeneration();

	/**
	*  @brief
	*    Fill command buffer content processing
	*
	*  @note
	*    - When this method is called it's ensured that the renderer instance "mRenderer" is valid
	*/
	void fillCommandBufferContentProcessing();

	/**
	*  @brief
	*    Called on application should to its job
	*
	*  @note
	*    - When this method is called it's ensured that the renderer instance "mRenderer" is valid
	*/
	void onDoJob();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	char						 mRendererName[32];		///< Case sensitive ASCII name of the renderer to instance
	Renderer::RendererInstance*	 mRendererInstance;		///< Renderer instance, can be a null pointer
	Renderer::IRendererPtr		 mRenderer;				///< Renderer instance, can be a null pointer
	Renderer::IBufferManagerPtr	 mBufferManager;		///< Buffer manager, can be a null pointer
	Renderer::ITextureManagerPtr mTextureManager;		///< Buffer manager, can be a null pointer
	Renderer::IRootSignaturePtr	 mRootSignature;		///< Root signature, can be a null pointer
	Renderer::ITexture2DPtr		 mTexture2D[2];			///< 2D texture, can be a null pointer
	Renderer::IFramebufferPtr	 mFramebuffer[2];		///< Framebuffer object (FBO), can be a null pointer
	Renderer::IResourceGroupPtr	 mTextureGroup;			///< Texture group, can be a null pointer
	Renderer::IResourceGroupPtr	 mSamplerStateGroup;	///< Sampler state resource group, can be a null pointer
	// Content generation
	Renderer::IPipelineStatePtr  mPipelineStateContentGeneration;	///< Pipeline state object (PSO) for content generation, can be a null pointer
	Renderer::IVertexArrayPtr    mVertexArrayContentGeneration;		///< Vertex array object (VAO) for content generation, can be a null pointer
	Renderer::CommandBuffer		 mCommandBufferContentGeneration;	///< Command buffer for content generation
	// Content processing
	Renderer::IPipelineStatePtr  mPipelineStateContentProcessing;	///< Pipeline state object (PSO) for content processing, can be a null pointer
	Renderer::IVertexArrayPtr    mVertexArrayContentProcessing;		///< Vertex array object (VAO) for content processing, can be a null pointer
	Renderer::CommandBuffer		 mCommandBufferContentProcessing;	///< Command buffer for content processing


};
