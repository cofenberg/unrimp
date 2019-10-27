/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "Examples/Private/Framework/ExampleBase.h"

#include <Rhi/Public/Rhi.h>


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    A first example showing how to render into multiple render targets (MRT)
*
*  @remarks
*    Demonstrates:
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - 2D texture
*    - Sampler state object
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Graphics pipeline state object (PSO)
*    - Framebuffer object (FBO) used for render to texture
*    - Multiple render targets (MRT)
*/
class FirstMultipleRenderTargets final : public ExampleBase
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	inline FirstMultipleRenderTargets()
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~FirstMultipleRenderTargets() override
	{
		// The resources are released within "onDeinitialization()"
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onDraw() override;


//[-------------------------------------------------------]
//[ Private definitions                                   ]
//[-------------------------------------------------------]
private:
	static constexpr uint32_t TEXTURE_SIZE		 = 16;	///< Texture size
	static constexpr uint32_t NUMBER_OF_TEXTURES = 2;	///< Number of textures


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	void fillCommandBuffer();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Rhi::IBufferManagerPtr			mBufferManager;									///< Buffer manager, can be a null pointer
	Rhi::ITextureManagerPtr			mTextureManager;								///< Texture manager, can be a null pointer
	Rhi::CommandBuffer				mCommandBuffer;									///< Command buffer
	Rhi::IRootSignaturePtr			mRootSignature;									///< Root signature, can be a null pointer
	Rhi::IFramebufferPtr			mFramebuffer;									///< Framebuffer object (FBO), can be a null pointer
	Rhi::IResourceGroupPtr			mTextureGroup;									///< Texture group, can be a null pointer
	Rhi::IResourceGroupPtr			mSamplerStateGroup;								///< Sampler state resource group, can be a null pointer
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineStateMultipleRenderTargets;	///< Graphics pipeline state object (PSO) multiple render targets, can be a null pointer
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineState;							///< Graphics pipeline state object (PSO), can be a null pointer
	Rhi::IVertexArrayPtr			mVertexArray;									///< Vertex array object (VAO), can be a null pointer


};
