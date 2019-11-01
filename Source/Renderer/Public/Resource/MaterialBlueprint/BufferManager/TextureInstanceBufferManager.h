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
#include "Renderer/Public/Core/Manager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class Renderable;
	class IRenderer;
	class MaterialTechnique;
	class PassBufferManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Texture instance buffer manager; includes a texture buffer to transport more complex per-instance data
	*
	*  @remarks
	*    "Renderer::UniformInstanceBufferManager" is a simplified version of "Renderer::TextureInstanceBufferManager". Shared code is duplicated by intent
	*     (including this comment) to avoid making the implementations too complex due to over-engineering. This is performance critical code and the topic is complex
	*     enough as it is. When changing one implementation don't forget to update the other one as well.
	*/
	class TextureInstanceBufferManager final : private Manager
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
		*/
		explicit TextureInstanceBufferManager(IRenderer& renderer);

		/**
		*  @brief
		*    Destructor
		*/
		~TextureInstanceBufferManager();

		/**
		*  @brief
		*    Startup instance buffer filling
		*
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*/
		void startupBufferFilling(const MaterialBlueprintResource& materialBlueprintResource, Rhi::CommandBuffer& commandBuffer);

		/**
		*  @brief
		*    Fill the instance buffer
		*
		*  @param[in] worldSpaceCameraPosition
		*    64 bit world space position of the camera for camera relative rendering
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource
		*  @param[in] passBufferManager
		*    Pass buffer manager instance to use, can be a null pointer
		*  @param[in] instanceUniformBuffer
		*    Instance uniform buffer instance to use
		*  @param[in] renderable
		*    Renderable to fill the buffer for
		*  @param[in] materialTechnique
		*    Used material technique
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*
		*  @return
		*    Start instance location, used for draw ID (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
		*/
		[[nodiscard]] uint32_t fillBuffer(const glm::dvec3& worldSpaceCameraPosition, const MaterialBlueprintResource& materialBlueprintResource, PassBufferManager* passBufferManager, const MaterialBlueprintResource::UniformBuffer& instanceUniformBuffer, const Renderable& renderable, MaterialTechnique& materialTechnique, Rhi::CommandBuffer& commandBuffer);

		/**
		*  @brief
		*    Called pre command buffer execution
		*/
		void onPreCommandBufferExecution();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureInstanceBufferManager(const TextureInstanceBufferManager&) = delete;
		TextureInstanceBufferManager& operator=(const TextureInstanceBufferManager&) = delete;
		void createInstanceBuffer();
		void mapCurrentInstanceBuffer();
		void unmapCurrentInstanceBuffer();


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct InstanceBuffer final
		{
			Rhi::IUniformBuffer* uniformBuffer;	///< RHI uniform buffer instance, always valid
			Rhi::ITextureBuffer* textureBuffer;	///< RHI texture buffer instance, always valid
			Rhi::IResourceGroup* resourceGroup;	///< RHI resource group instance, can be a null pointer
			bool				 mapped;
			InstanceBuffer(Rhi::IUniformBuffer& _uniformBuffer, Rhi::ITextureBuffer& _textureBuffer) :
				uniformBuffer(&_uniformBuffer),
				textureBuffer(&_textureBuffer),
				resourceGroup(nullptr),
				mapped(false)
			{
				// Nothing here
			}

		};
		typedef std::vector<InstanceBuffer> InstanceBuffers;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRenderer&		mRenderer;						///< Renderer instance to use
		const uint32_t	mMaximumUniformBufferSize;		///< Maximum uniform buffer size in bytes
		const uint32_t	mMaximumTextureBufferSize;		///< Maximum texture buffer size in bytes
		InstanceBuffers	mInstanceBuffers;				///< Instance buffers
		// Current instance buffer related data
		size_t			mCurrentInstanceBufferIndex;	///< Current instance buffer index, can be invalid if there's currently no current instance buffer
		InstanceBuffer* mCurrentInstanceBuffer;			///< Current instance buffer, can be a null pointer, don't destroy the instance since this is just a reference
		uint8_t*		mStartUniformBufferPointer;
		uint8_t*		mCurrentUniformBufferPointer;
		float*			mStartTextureBufferPointer;
		float*			mCurrentTextureBufferPointer;
		uint32_t		mStartInstanceLocation;			///< Start instance location, used for draw ID (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
