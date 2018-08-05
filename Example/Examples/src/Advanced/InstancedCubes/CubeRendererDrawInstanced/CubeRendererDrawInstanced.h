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
#include "Advanced/InstancedCubes/ICubeRenderer.h"

#include <Renderer/Renderer.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class BatchDrawInstanced;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Cube renderer class using instancing
*
*  @remarks
*    Required renderer features:
*    - Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
*    - 2D array texture
*    - Texture buffer
*/
class CubeRendererDrawInstanced final : public ICubeRenderer
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] renderer
	*    Renderer instance to use
	*  @param[in] renderPass
	*    Render pass to use
	*  @param[in] numberOfTextures
	*    Number of textures, must be <ICubeRenderer::MAXIMUM_NUMBER_OF_TEXTURES
	*  @param[in] sceneRadius
	*    Scene radius
	*/
	CubeRendererDrawInstanced(Renderer::IRenderer& renderer, Renderer::IRenderPass& renderPass, uint32_t numberOfTextures, uint32_t sceneRadius);

	/**
	*  @brief
	*    Destructor
	*/
	virtual ~CubeRendererDrawInstanced() override;


//[-------------------------------------------------------]
//[ Public virtual ICubeRenderer methods                  ]
//[-------------------------------------------------------]
public:
	virtual void setNumberOfCubes(uint32_t numberOfCubes) override;
	virtual void fillCommandBuffer(float globalTimer, float globalScale, float lightPositionX, float lightPositionY, float lightPositionZ, Renderer::CommandBuffer& commandBuffer) override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit CubeRendererDrawInstanced(const CubeRendererDrawInstanced& source) = delete;
	CubeRendererDrawInstanced& operator =(const CubeRendererDrawInstanced& source) = delete;
	void fillReusableCommandBuffer();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Renderer::IRendererPtr		  mRenderer;							///< Renderer instance to use, always valid
	Renderer::IRenderPass&		  mRenderPass;							///< Render pass to use, always valid
	Renderer::IBufferManagerPtr	  mBufferManager;						///< Buffer manager, can be a null pointer
	Renderer::ITextureManagerPtr  mTextureManager;						///< Texture manager, can be a null pointer
	Renderer::CommandBuffer		  mCommandBuffer;						///< Command buffer which is recorded once and then used multiple times
	uint32_t					  mNumberOfTextures;					///< Number of textures
	uint32_t					  mSceneRadius;							///< Scene radius
	uint32_t					  mMaximumNumberOfInstancesPerBatch;	///< Maximum number of instances per batch
	Renderer::IRootSignaturePtr	  mRootSignature;						///< Root signature, can be a null pointer
	Renderer::ITexture2DArrayPtr  mTexture2DArray;						///< 2D texture array, can be a null pointer
	Renderer::IUniformBufferPtr	  mUniformBufferStaticVs;				///< Static vertex shader uniform buffer object (UBO), can be a null pointer
	Renderer::IUniformBufferPtr	  mUniformBufferDynamicVs;				///< Dynamic vertex shader uniform buffer object (UBO), can be a null pointer
	Renderer::IUniformBufferPtr	  mUniformBufferDynamicFs;				///< Dynamic fragment shader uniform buffer object (UBO), can be a null pointer
	Renderer::IResourceGroupPtr	  mResourceGroup;						///< Resource group, can be a null pointer
	Renderer::IResourceGroupPtr	  mSamplerStateGroup;					///< Sampler state resource group, can be a null pointer
	Renderer::IGraphicsProgramPtr mGraphicsProgram;						///< Graphics program, can be a null pointer
	Renderer::IVertexArrayPtr	  mVertexArray;							///< Vertex array object (VAO), can be a null pointer
	uint32_t					  mNumberOfBatches;						///< Current number of batches
	BatchDrawInstanced*			  mBatches;								///< Batches, can be a null pointer


};
