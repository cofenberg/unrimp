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
#include "Renderer/Public/Export.h"

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
namespace Renderer
{
	class RenderableManager;
	class MaterialResourceManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier
	typedef uint32_t SkeletonResourceId;	///< POD skeleton resource identifier
	typedef uint32_t MaterialTechniqueId;	///< Material technique identifier, result of hashing the material technique name via "Renderer::StringId"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Renderable
	*
	*  @note
	*    - Example: Abstract representation of a sub-mesh which is part of an mesh scene item
	*/
	class Renderable final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RenderQueue;		// Must be able to update cached material data
		friend class MaterialResource;	// Must be able to update cached material data


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		RENDERER_API_EXPORT Renderable();
		RENDERER_API_EXPORT Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, uint32_t startIndexLocation, uint32_t numberOfIndices, uint32_t instanceCount = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER);
		RENDERER_API_EXPORT Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, const Rhi::IIndirectBufferPtr& indirectBufferPtr, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER);
		RENDERER_API_EXPORT Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const Rhi::IVertexArrayPtr& positionOnlyVertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, uint32_t startIndexLocation, uint32_t numberOfIndices, uint32_t instanceCount = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER);
		RENDERER_API_EXPORT Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const Rhi::IVertexArrayPtr& positionOnlyVertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, const Rhi::IIndirectBufferPtr& indirectBufferPtr, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER);

		inline ~Renderable()
		{
			unsetMaterialResourceIdInternal();
		}

		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RHI_DEBUG
			/**
			*  @brief
			*    Return the renderable debug name
			*
			*  @return
			*    The renderable debug name, never a null pointer and at least an empty string
			*
			*  @note
			*    - If possible, the renderable debug name should use the following convention: "<filename>?[<attribute 0>][<attribute n>]" (for "?" see "Renderer::IFileManager::INVALID_CHARACTER")
			*/
			[[nodiscard]] inline const char* getDebugName() const
			{
				return mDebugName;
			}

			inline void setDebugName(const char debugName[])
			{
				ASSERT(strlen(debugName) < 256, "Renderable debug name is not allowed to exceed 255 characters")
				strncpy(mDebugName, debugName, 256);
				mDebugName[255] = '\0';
			}
		#endif

		//[-------------------------------------------------------]
		//[ Data                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline RenderableManager& getRenderableManager() const
		{
			return mRenderableManager;
		}

		[[nodiscard]] inline const Rhi::IVertexArrayPtr& getVertexArrayPtr() const
		{
			return mVertexArrayPtr;
		}

		[[nodiscard]] inline const Rhi::IVertexArrayPtr& getPositionOnlyVertexArrayPtr() const
		{
			return mPositionOnlyVertexArrayPtr;
		}

		[[nodiscard]] inline const Rhi::IVertexArrayPtr& getPositionOnlyVertexArrayPtrWithFallback() const
		{
			// In case an alpha map is used, usage of the position-only vertex array object (VAO) can result in visual artifacts since also a texture coordinate is needed for sampling
			return (!mUseAlphaMap && mPositionOnlyVertexArrayPtr != nullptr) ? mPositionOnlyVertexArrayPtr : mVertexArrayPtr;
		}

		inline void setVertexArrayPtr(const Rhi::IVertexArrayPtr& vertexArrayPtr, const Rhi::IVertexArrayPtr& positionOnlyVertexArrayPtr = Rhi::IVertexArrayPtr())
		{
			mVertexArrayPtr = vertexArrayPtr;
			mPositionOnlyVertexArrayPtr = positionOnlyVertexArrayPtr;
		}

		[[nodiscard]] inline const Rhi::IIndirectBufferPtr& getIndirectBufferPtr() const
		{
			return mIndirectBufferPtr;
		}

		inline void setIndirectBufferPtr(const Rhi::IIndirectBufferPtr& indirectBufferPtr)
		{
			mIndirectBufferPtr = indirectBufferPtr;
		}

		[[nodiscard]] inline bool getDrawIndexed() const
		{
			return mDrawIndexed;
		}

		inline void setDrawIndexed(bool drawIndexed)
		{
			mDrawIndexed = drawIndexed;
		}

		[[nodiscard]] inline uint32_t getStartIndexLocation() const
		{
			return mStartIndexLocation;
		}

		inline void setStartIndexLocation(uint32_t startIndexLocation)
		{
			mStartIndexLocation = startIndexLocation;
		}

		[[nodiscard]] inline uint32_t getIndirectBufferOffset() const
		{
			return mIndirectBufferOffset;
		}

		inline void setIndirectBufferOffset(uint32_t indirectBufferOffset)
		{
			mIndirectBufferOffset = indirectBufferOffset;
		}

		[[nodiscard]] inline uint32_t getNumberOfIndices() const
		{
			return mNumberOfIndices;
		}

		inline void setNumberOfIndices(uint32_t numberOfIndices)
		{
			mNumberOfIndices = numberOfIndices;
		}

		[[nodiscard]] inline uint32_t getInstanceCount() const
		{
			return mInstanceCount;
		}

		inline void setInstanceCount(uint32_t instanceCount)
		{
			mInstanceCount = instanceCount;
		}

		[[nodiscard]] inline uint32_t getNumberOfDraws() const
		{
			return mNumberOfDraws;
		}

		inline void setNumberOfDraws(uint32_t numberOfDraws)
		{
			mNumberOfDraws = numberOfDraws;
		}

		[[nodiscard]] inline MaterialResourceId getMaterialResourceId() const
		{
			return mMaterialResourceId;
		}

		RENDERER_API_EXPORT void setMaterialResourceId(const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId);

		inline void unsetMaterialResourceId()
		{
			unsetMaterialResourceIdInternal();
		}

		[[nodiscard]] inline SkeletonResourceId getSkeletonResourceId() const
		{
			return mSkeletonResourceId;
		}

		inline void setSkeletonResourceId(SkeletonResourceId skeletonResourceId)
		{
			mSkeletonResourceId = skeletonResourceId;
		}

		//[-------------------------------------------------------]
		//[ Cached material data                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline uint8_t getRenderQueueIndex() const
		{
			return mRenderQueueIndex;
		}

		[[nodiscard]] inline bool getCastShadows() const
		{
			return mCastShadows;
		}

		[[nodiscard]] inline bool getUseAlphaMap() const
		{
			return mUseAlphaMap;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		Renderable& operator=(const Renderable&) = delete;
		RENDERER_API_EXPORT void unsetMaterialResourceIdInternal();


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct PipelineStateCache
		{
			MaterialTechniqueId	   materialTechniqueId;
			uint32_t			   generationCounter;	// Most simple solution to detect e.g. shader combination changes which make the pipeline state cache invalid
			Rhi::IPipelineStatePtr pipelineStatePtr;
			inline PipelineStateCache(MaterialTechniqueId _materialTechniqueId, uint32_t _generationCounter, const Rhi::IGraphicsPipelineStatePtr& graphicsPipelineStatePtr) :
				materialTechniqueId(_materialTechniqueId),
				generationCounter(_generationCounter),
				pipelineStatePtr(graphicsPipelineStatePtr)
			{};
			inline PipelineStateCache(MaterialTechniqueId _materialTechniqueId, uint32_t _generationCounter, const Rhi::IComputePipelineStatePtr& computePipelineStatePtr) :
				materialTechniqueId(_materialTechniqueId),
				generationCounter(_generationCounter),
				pipelineStatePtr(computePipelineStatePtr)
			{};
		};
		typedef std::vector<PipelineStateCache> PipelineStateCaches;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Debug
		#ifdef RHI_DEBUG
			char						mDebugName[256];				///< Debug name for easier renderable identification when debugging, contains terminating zero, first member variable by intent to see it at once during introspection (debug memory layout change is no problem here)
		#endif
		// Data
		RenderableManager&				mRenderableManager;
		Rhi::IVertexArrayPtr			mVertexArrayPtr;				///< Optional vertex array object (VAO), can be a null pointer
		Rhi::IVertexArrayPtr			mPositionOnlyVertexArrayPtr;	///< Optional position-only vertex array object (VAO) which can reduce the number of processed vertices up to half, can be a null pointer, can be used for position-only rendering (e.g. shadow map rendering) using the same vertex data that the original vertex array object (VAO) uses
		Rhi::IIndirectBufferPtr			mIndirectBufferPtr;				///< Optional indirect buffer, can be a null pointer
		union
		{
			uint32_t					mStartIndexLocation;			///< Used in case there's no indirect buffer
			uint32_t					mIndirectBufferOffset;			///< Used in case there's an indirect buffer
		};
		uint32_t						mNumberOfIndices;				///< If there's an indirect buffer set, this value is unused
		union
		{
			uint32_t					mInstanceCount;					///< Used in case there's no indirect buffer
			uint32_t					mNumberOfDraws;					///< Used in case there's an indirect buffer
		};
		MaterialResourceId				mMaterialResourceId;
		SkeletonResourceId				mSkeletonResourceId;
		bool							mDrawIndexed;					///< Placed at this location due to padding
		// Cached material data
		uint8_t							mRenderQueueIndex;
		bool							mCastShadows;
		bool							mUseAlphaMap;					///< Relevant e.g. in combination with cast shadows in which the optional position-only vertex array object (VAO) can't be used in case an alpha mask is used
		PipelineStateCaches				mPipelineStateCaches;
		// Internal data
		const MaterialResourceManager*	mMaterialResourceManager;
		int								mMaterialResourceAttachmentIndex;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
