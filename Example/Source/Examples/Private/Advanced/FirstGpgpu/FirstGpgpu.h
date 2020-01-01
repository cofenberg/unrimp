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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class RhiInstance;
}
class ExampleRunner;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    A first example showing how to use do General Purpose Computation on Graphics Processing Unit (GPGPU) by using the RHI and shaders without having any output window (also known as headless rendering)
*
*  @remarks
*    Demonstrates:
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - 2D texture
*    - Sampler state object (SO)
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Graphics pipeline state object (PSO)
*    - Framebuffer object (FBO) used for render to texture
*    - General Purpose Computation on Graphics Processing Unit (GPGPU) by using the RHI and shaders without having any output window
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
	*  @param[in] exampleRunner
	*    Example runner
	*  @param[in] rhiName
	*    Case sensitive ASCII name of the RHI to instance, if null pointer or unknown RHI no RHI will be used.
	*    Example RHI names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*/
	FirstGpgpu(ExampleRunner& exampleRunner, const char* rhiName);

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
	[[nodiscard]] int run();


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	FirstGpgpu& operator=(const FirstGpgpu& firstGpgpu) = delete;

	/**
	*  @brief
	*    Called on application initialization
	*
	*  @note
	*    - When this method is called it's ensured that the RHI instance "mRhi" is valid
	*/
	void onInitialization();

	/**
	*  @brief
	*    Called on application de-initialization
	*
	*  @note
	*    - When this method is called it's ensured that the RHI instance "mRhi" is valid
	*/
	void onDeinitialization();

	/**
	*  @brief
	*    Fill command buffer content generation
	*
	*  @note
	*    - When this method is called it's ensured that the RHI instance "mRhi" is valid
	*/
	void fillCommandBufferContentGeneration();

	/**
	*  @brief
	*    Fill command buffer content processing
	*
	*  @note
	*    - When this method is called it's ensured that the RHI instance "mRhi" is valid
	*/
	void fillCommandBufferContentProcessing();

	/**
	*  @brief
	*    Called on application should to its job
	*
	*  @note
	*    - When this method is called it's ensured that the RHI instance "mRhi" is valid
	*/
	void onDoJob();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	ExampleRunner&			mExampleRunner;
	char					mRhiName[32];		///< Case sensitive ASCII name of the RHI to instance
	Rhi::RhiInstance*		mRhiInstance;		///< RHI instance, can be a null pointer
	Rhi::IRhiPtr			mRhi;				///< RHI instance, can be a null pointer
	Rhi::IBufferManagerPtr	mBufferManager;		///< Buffer manager, can be a null pointer
	Rhi::ITextureManagerPtr mTextureManager;	///< Buffer manager, can be a null pointer
	Rhi::IRootSignaturePtr	mRootSignature;		///< Root signature, can be a null pointer
	Rhi::ITexture2DPtr		mTexture2D[2];		///< 2D texture, can be a null pointer
	Rhi::IFramebufferPtr	mFramebuffer[2];	///< Framebuffer object (FBO), can be a null pointer
	Rhi::IResourceGroupPtr	mTextureGroup;		///< Texture group, can be a null pointer
	Rhi::IResourceGroupPtr	mSamplerStateGroup;	///< Sampler state resource group, can be a null pointer
	// Content generation
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineStateContentGeneration;	///< Graphics pipeline state object (PSO) for content generation, can be a null pointer
	Rhi::IVertexArrayPtr			mVertexArrayContentGeneration;				///< Vertex array object (VAO) for content generation, can be a null pointer
	Rhi::CommandBuffer				mCommandBufferContentGeneration;			///< Command buffer for content generation
	// Content processing
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineStateContentProcessing;	///< Graphics pipeline state object (PSO) for content processing, can be a null pointer
	Rhi::IVertexArrayPtr			mVertexArrayContentProcessing;				///< Vertex array object (VAO) for content processing, can be a null pointer
	Rhi::CommandBuffer				mCommandBufferContentProcessing;			///< Command buffer for content processing


};
