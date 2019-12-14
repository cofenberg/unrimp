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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/RenderQueue/Renderable.h"
#include "Renderer/Public/RenderQueue/RenderableManager.h"
#include "Renderer/Public/Resource/Material/MaterialResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialResource.h"
#include "Renderer/Public/Core/SwizzleVectorElementRemove.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		Renderer::RenderableManager NullRenderableManager;


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Renderable::Renderable() :
		// Debug
		#ifdef RHI_DEBUG
			mDebugName{},
		#endif
		// Data
		mRenderableManager(::detail::NullRenderableManager),
		mStartIndexLocation(0),
		mNumberOfIndices(0),
		mInstanceCount(1),
		mMaterialResourceId(getInvalid<MaterialResourceId>()),
		mSkeletonResourceId(getInvalid<SkeletonResourceId>()),
		mDrawIndexed(false),
		// Cached material data
		mRenderQueueIndex(0),
		mCastShadows(false),
		mUseAlphaMap(false),
		// Internal data
		mMaterialResourceManager(nullptr),
		mMaterialResourceAttachmentIndex(getInvalid<int>())
	{
		// Nothing here
	}

	Renderable::Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, uint32_t startIndexLocation, uint32_t numberOfIndices, uint32_t instanceCount RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
		// Debug
		#ifdef RHI_DEBUG
			mDebugName{},
		#endif
		// Data
		mRenderableManager(renderableManager),
		mVertexArrayPtr(vertexArrayPtr),
		mStartIndexLocation(startIndexLocation),
		mNumberOfIndices(numberOfIndices),
		mInstanceCount(instanceCount),
		mMaterialResourceId(getInvalid<MaterialResourceId>()),
		mSkeletonResourceId(skeletonResourceId),
		mDrawIndexed(drawIndexed),
		// Cached material data
		mRenderQueueIndex(0),
		mCastShadows(false),
		mUseAlphaMap(false),
		// Internal data
		mMaterialResourceManager(nullptr),
		mMaterialResourceAttachmentIndex(getInvalid<int>())
	{
		#ifdef RHI_DEBUG
			if ('\0' != debugName[0])
			{
				setDebugName(debugName);
			}
		#endif
		if (isValid(materialResourceId))
		{
			setMaterialResourceId(materialResourceManager, materialResourceId);
		}
	}

	Renderable::Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, const Rhi::IIndirectBufferPtr& indirectBufferPtr, uint32_t indirectBufferOffset, uint32_t numberOfDraws RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
		// Debug
		#ifdef RHI_DEBUG
			mDebugName{},
		#endif
		// Data
		mRenderableManager(renderableManager),
		mVertexArrayPtr(vertexArrayPtr),
		mIndirectBufferPtr(indirectBufferPtr),
		mIndirectBufferOffset(indirectBufferOffset),	// Indirect buffer used
		mNumberOfIndices(~0u),							// Invalid since read from the indirect buffer
		mNumberOfDraws(numberOfDraws),					// Indirect buffer used
		mMaterialResourceId(getInvalid<MaterialResourceId>()),
		mSkeletonResourceId(skeletonResourceId),
		mDrawIndexed(drawIndexed),
		// Cached material data
		mRenderQueueIndex(0),
		mCastShadows(false),
		mUseAlphaMap(false),
		// Internal data
		mMaterialResourceManager(nullptr),
		mMaterialResourceAttachmentIndex(getInvalid<int>())
	{
		#ifdef RHI_DEBUG
			if ('\0' != debugName[0])
			{
				setDebugName(debugName);
			}
		#endif
		if (isValid(materialResourceId))
		{
			setMaterialResourceId(materialResourceManager, materialResourceId);
		}
	}

	Renderable::Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const Rhi::IVertexArrayPtr& positionOnlyVertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, uint32_t startIndexLocation, uint32_t numberOfIndices, uint32_t instanceCount RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
		// Debug
		#ifdef RHI_DEBUG
			mDebugName{},
		#endif
		// Data
		mRenderableManager(renderableManager),
		mVertexArrayPtr(vertexArrayPtr),
		mPositionOnlyVertexArrayPtr(positionOnlyVertexArrayPtr),
		mStartIndexLocation(startIndexLocation),
		mNumberOfIndices(numberOfIndices),
		mInstanceCount(instanceCount),
		mMaterialResourceId(getInvalid<MaterialResourceId>()),
		mSkeletonResourceId(skeletonResourceId),
		mDrawIndexed(drawIndexed),
		// Cached material data
		mRenderQueueIndex(0),
		mCastShadows(false),
		mUseAlphaMap(false),
		// Internal data
		mMaterialResourceManager(nullptr),
		mMaterialResourceAttachmentIndex(getInvalid<int>())
	{
		#ifdef RHI_DEBUG
			if ('\0' != debugName[0])
			{
				setDebugName(debugName);
			}
		#endif
		if (isValid(materialResourceId))
		{
			setMaterialResourceId(materialResourceManager, materialResourceId);
		}
	}

	Renderable::Renderable(RenderableManager& renderableManager, const Rhi::IVertexArrayPtr& vertexArrayPtr, const Rhi::IVertexArrayPtr& positionOnlyVertexArrayPtr, const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId, SkeletonResourceId skeletonResourceId, bool drawIndexed, const Rhi::IIndirectBufferPtr& indirectBufferPtr, uint32_t indirectBufferOffset, uint32_t numberOfDraws RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
		// Debug
		#ifdef RHI_DEBUG
			mDebugName{},
		#endif
		// Data
		mRenderableManager(renderableManager),
		mVertexArrayPtr(vertexArrayPtr),
		mPositionOnlyVertexArrayPtr(positionOnlyVertexArrayPtr),
		mIndirectBufferPtr(indirectBufferPtr),
		mIndirectBufferOffset(indirectBufferOffset),	// Indirect buffer used
		mNumberOfIndices(~0u),							// Invalid since read from the indirect buffer
		mNumberOfDraws(numberOfDraws),					// Indirect buffer used
		mMaterialResourceId(getInvalid<MaterialResourceId>()),
		mSkeletonResourceId(skeletonResourceId),
		mDrawIndexed(drawIndexed),
		// Cached material data
		mRenderQueueIndex(0),
		mCastShadows(false),
		mUseAlphaMap(false),
		// Internal data
		mMaterialResourceManager(nullptr),
		mMaterialResourceAttachmentIndex(getInvalid<int>())
	{
		#ifdef RHI_DEBUG
			if ('\0' != debugName[0])
			{
				setDebugName(debugName);
			}
		#endif
		if (isValid(materialResourceId))
		{
			setMaterialResourceId(materialResourceManager, materialResourceId);
		}
	}

	void Renderable::setMaterialResourceId(const MaterialResourceManager& materialResourceManager, MaterialResourceId materialResourceId)
	{
		// State change?
		if (mMaterialResourceId != materialResourceId)
		{
			// Detach the renderable from the previous material resource, first
			unsetMaterialResourceIdInternal();

			// Renderables can only be attached to really existing material resources
			MaterialResource* materialResource = materialResourceManager.tryGetById(materialResourceId);
			if (nullptr != materialResource)
			{
				// Sanity checks
				RHI_ASSERT(materialResourceManager.getRenderer().getContext(), isInvalid(mMaterialResourceAttachmentIndex), "Invalid material resource attachment index")
				RHI_ASSERT(materialResourceManager.getRenderer().getContext(), nullptr == mMaterialResourceManager, "Invalid material resource manager instance")

				// Attach the renderable from the material resource
				mMaterialResourceId = materialResourceId;
				mMaterialResourceManager = &materialResourceManager;
				mMaterialResourceAttachmentIndex = static_cast<int>(materialResource->mAttachedRenderables.size());
				materialResource->mAttachedRenderables.push_back(this);

				{ // Cached material data, incremental updates are handled inside "Renderer::MaterialResource::setPropertyByIdInternal()"
					// Optional "RenderQueueIndex" (e.g. compositor materials usually don't need this property)
					const MaterialProperty* materialProperty = materialResource->getPropertyById(MaterialResource::RENDER_QUEUE_INDEX_PROPERTY_ID);
					if (nullptr != materialProperty)
					{
						const int renderQueueIndex = materialProperty->getIntegerValue();

						// Sanity checks
						RHI_ASSERT(materialResourceManager.getRenderer().getContext(), materialProperty->getUsage() == MaterialProperty::Usage::STATIC, "Invalid material property usage")
						RHI_ASSERT(materialResourceManager.getRenderer().getContext(), renderQueueIndex >= 0, "Invalid render queue index")
						RHI_ASSERT(materialResourceManager.getRenderer().getContext(), renderQueueIndex <= 255, "Invalid render queue index")

						// Set value
						mRenderQueueIndex = static_cast<uint8_t>(renderQueueIndex);
					}
					else
					{
						mRenderQueueIndex = 0;
					}

					// Optional "CastShadows" (e.g. compositor materials usually don't need this property)
					materialProperty = materialResource->getPropertyById(MaterialResource::CAST_SHADOWS_PROPERTY_ID);
					if (nullptr != materialProperty)
					{
						// Sanity checks
						RHI_ASSERT(materialResourceManager.getRenderer().getContext(), materialProperty->getUsage() == MaterialProperty::Usage::STATIC, "Invalid material property usage")

						// Set value
						mCastShadows = materialProperty->getBooleanValue();
					}
					else
					{
						mCastShadows = false;
					}

					// Optional "UseAlphaMap"
					materialProperty = materialResource->getPropertyById(MaterialResource::USE_ALPHA_MAP_PROPERTY_ID);
					if (nullptr != materialProperty)
					{
						// Sanity checks
						RHI_ASSERT(materialResourceManager.getRenderer().getContext(), materialProperty->getUsage() == MaterialProperty::Usage::SHADER_COMBINATION, "Invalid material property usage")

						// Set value
						mUseAlphaMap = materialProperty->getBooleanValue();
					}
					else
					{
						mUseAlphaMap = false;
					}
				}
			}
			else
			{
				// Error!
				RHI_ASSERT(materialResourceManager.getRenderer().getContext(), false, "We should never end up in here")
			}
		}
		else
		{
			// Sanity check
			RHI_ASSERT(materialResourceManager.getRenderer().getContext(), (isValid(mMaterialResourceId) && &materialResourceManager == mMaterialResourceManager) || (isInvalid(mMaterialResourceId) && nullptr == mMaterialResourceManager), "Invalid renderable configuration")
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void Renderable::unsetMaterialResourceIdInternal()
	{
		if (isValid(mMaterialResourceId))
		{
			// Sanity checks
			ASSERT(nullptr != mMaterialResourceManager);
			ASSERT(isValid(mMaterialResourceAttachmentIndex));

			// Get the material resource we're going to detach from
			MaterialResource& materialResource = mMaterialResourceManager->getById(mMaterialResourceId);

			// Sanity checks
			ASSERT(mMaterialResourceAttachmentIndex < static_cast<int>(materialResource.mAttachedRenderables.size()));
			ASSERT(this == *(materialResource.mAttachedRenderables.begin() + mMaterialResourceAttachmentIndex));
			ASSERT(materialResource.getId() == mMaterialResourceId);

			// Detach the renderable from the material resource
			MaterialResource::AttachedRenderables::iterator iterator = materialResource.mAttachedRenderables.begin() + mMaterialResourceAttachmentIndex;
			iterator = ::detail::swizzleVectorElementRemove(materialResource.mAttachedRenderables, iterator);
			if (iterator != materialResource.mAttachedRenderables.end())
			{
				// The node that was at the end got swapped and has now a different index
				(*iterator)->mMaterialResourceAttachmentIndex = static_cast<int>(iterator - materialResource.mAttachedRenderables.begin());
			}
			setInvalid(mMaterialResourceId);
			mMaterialResourceManager = nullptr;
			setInvalid(mMaterialResourceAttachmentIndex);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
