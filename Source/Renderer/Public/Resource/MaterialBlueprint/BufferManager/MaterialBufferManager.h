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
#include "Renderer/Public/Core/Manager.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class CommandBuffer;
	class IUniformBuffer;
	class IBufferManager;
	class IResourceGroup;
}
namespace Renderer
{
	class IRenderer;
	class MaterialBufferSlot;
	class MaterialBlueprintResource;
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
	*    Material buffer manager
	*
	*  @note
	*    - For material batching
	*    - Concept basing on OGRE 2.1 "Ogre::ConstBufferPool", but more generic and simplified thanks to the material blueprint concept
	*/
	class MaterialBufferManager final : private Manager
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
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource
		*/
		MaterialBufferManager(IRenderer& renderer, const MaterialBlueprintResource& materialBlueprintResource);

		/**
		*  @brief
		*    Destructor
		*/
		~MaterialBufferManager();

		/**
		*  @brief
		*    Request a slot and fill the material slot; automatically schedules for update
		*/
		void requestSlot(MaterialBufferSlot& materialBufferSlot);

		/**
		*  @brief
		*    Release a slot requested with "Renderer::MaterialBufferManager::requestSlot()"
		*/
		void releaseSlot(MaterialBufferSlot& materialBufferSlot);

		/**
		*  @brief
		*    Schedule the slot of the given material slot for update
		*/
		void scheduleForUpdate(MaterialBufferSlot& materialBufferSlot);

		/**
		*  @brief
		*    Reset last graphics bound pool and update the dirty slots
		*/
		void resetLastGraphicsBoundPool();

		/**
		*  @brief
		*    Reset last compute bound pool and update the dirty slots
		*/
		void resetLastComputeBoundPool();

		/**
		*  @brief
		*    Fill slot to graphics command buffer
		*
		*  @param[in] materialBufferSlot
		*    Graphics material buffer slot to bind
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*/
		void fillGraphicsCommandBuffer(MaterialBufferSlot& materialBufferSlot, Rhi::CommandBuffer& commandBuffer);

		/**
		*  @brief
		*    Fill slot to compute command buffer
		*
		*  @param[in] materialBufferSlot
		*    Compute material buffer slot to bind
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*/
		void fillComputeCommandBuffer(MaterialBufferSlot& materialBufferSlot, Rhi::CommandBuffer& commandBuffer);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MaterialBufferManager(const MaterialBufferManager&) = delete;
		MaterialBufferManager& operator=(const MaterialBufferManager&) = delete;
		void uploadDirtySlots();


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct BufferPool final
		{
			std::vector<uint32_t> freeSlots;
			Rhi::IUniformBuffer*  uniformBuffer;	///< Memory is managed by this buffer pool instance
			Rhi::IResourceGroup*  resourceGroup;	///< Memory is managed by this buffer pool instance

			BufferPool(uint32_t bufferSize, uint32_t slotsPerPool, Rhi::IBufferManager& bufferManager, const MaterialBlueprintResource& materialBlueprintResource);
			~BufferPool();
		};

		typedef std::vector<BufferPool*>		 BufferPools;
		typedef std::vector<MaterialBufferSlot*> MaterialBufferSlots;
		typedef std::vector<uint8_t>			 ScratchBuffer;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRenderer&						 mRenderer;
		const MaterialBlueprintResource& mMaterialBlueprintResource;
		BufferPools						 mBufferPools;
		uint32_t						 mSlotsPerPool;
		uint32_t						 mBufferSize;
		MaterialBufferSlots				 mDirtyMaterialBufferSlots;
		MaterialBufferSlots				 mMaterialBufferSlots;
		const BufferPool*				 mLastGraphicsBoundPool;
		const BufferPool*				 mLastComputeBoundPool;
		ScratchBuffer					 mScratchBuffer;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
