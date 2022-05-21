/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Examples/Private/Advanced/InstancedCubes/ICubeRenderer.h"

#include <Rhi/Public/Rhi.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class BatchInstancedArrays;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Cube renderer class using instancing
*
*  @remarks
*    Required RHI features:
*    - Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
*/
class CubeRendererInstancedArrays final : public ICubeRenderer
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] rhi
	*    RHI instance to use
	*  @param[in] renderPass
	*    Render pass to use
	*  @param[in] numberOfTextures
	*    Number of textures, must be <ICubeRenderer::MAXIMUM_NUMBER_OF_TEXTURES
	*  @param[in] sceneRadius
	*    Scene radius
	*/
	CubeRendererInstancedArrays(Rhi::IRhi& rhi, Rhi::IRenderPass& renderPass, uint32_t numberOfTextures, uint32_t sceneRadius);

	/**
	*  @brief
	*    Destructor
	*/
	virtual ~CubeRendererInstancedArrays() override;


//[-------------------------------------------------------]
//[ Public virtual ICubeRenderer methods                  ]
//[-------------------------------------------------------]
public:
	virtual void setNumberOfCubes(uint32_t numberOfCubes) override;
	virtual void fillCommandBuffer(float globalTimer, float globalScale, float lightPositionX, float lightPositionY, float lightPositionZ, Rhi::CommandBuffer& commandBuffer) override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit CubeRendererInstancedArrays(const CubeRendererInstancedArrays& source) = delete;
	CubeRendererInstancedArrays& operator =(const CubeRendererInstancedArrays& source) = delete;
	void fillReusableCommandBuffer();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Rhi::IRhiPtr			 mRhi;								///< RHI instance to use, always valid
	Rhi::IRenderPass&		 mRenderPass;						///< Render pass to use, always valid
	Rhi::IBufferManagerPtr	 mBufferManager;					///< Buffer manager, can be a null pointer
	Rhi::ITextureManagerPtr  mTextureManager;					///< Texture manager, can be a null pointer
	Rhi::CommandBuffer		 mCommandBuffer;					///< Command buffer which is recorded once and then used multiple times
	uint32_t				 mNumberOfTextures;					///< Number of textures
	uint32_t				 mSceneRadius;						///< Scene radius
	uint32_t				 mMaximumNumberOfInstancesPerBatch;	///< Maximum number of instances per batch
	Rhi::IRootSignaturePtr	 mRootSignature;					///< Root signature, can be a null pointer
	Rhi::ITexture2DPtr		 mTexture2D;						///< 2D texture, can be a null pointer
	Rhi::IUniformBufferPtr	 mUniformBufferStaticVs;			///< Static vertex shader uniform buffer object (UBO), can be a null pointer
	Rhi::IUniformBufferPtr	 mUniformBufferDynamicVs;			///< Dynamic vertex shader uniform buffer object (UBO), can be a null pointer
	Rhi::IUniformBufferPtr	 mUniformBufferDynamicFs;			///< Dynamic fragment shader uniform buffer object (UBO), can be a null pointer
	Rhi::IResourceGroupPtr	 mResourceGroupVs;					///< Resource group with vertex shader visibility, can be a null pointer
	Rhi::IResourceGroupPtr	 mResourceGroupFs;					///< Resource group with fragment shader visibility, can be a null pointer
	Rhi::IResourceGroupPtr	 mSamplerStateGroup;				///< Sampler state resource group, can be a null pointer
	Rhi::IGraphicsProgramPtr mGraphicsProgram;					///< Graphics program, can be a null pointer
	Rhi::IVertexBufferPtr	 mVertexBuffer;						///< Vertex buffer object (VBO), can be a null pointer
	Rhi::IIndexBufferPtr	 mIndexBuffer;						///< Index buffer object (IBO), can be a null pointer
	uint32_t				 mNumberOfBatches;					///< Current number of batches
	BatchInstancedArrays*	 mBatches;							///< Batches, can be a null pointer


};
