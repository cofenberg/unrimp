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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Resource/Material/MaterialTechnique.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Public/RenderQueue/RenderableManager.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Structures                                            ]
		//[-------------------------------------------------------]
		struct OrderByMaterialResourceId final
		{
			[[nodiscard]] inline bool operator()(RendererRuntime::MaterialResourceId left, RendererRuntime::MaterialResourceId right) const
			{
				return (left < right);
			}
		};

		struct OrderByMaterialTechniqueId final
		{
			[[nodiscard]] inline bool operator()(const RendererRuntime::MaterialTechnique* left, RendererRuntime::MaterialTechniqueId right) const
			{
				return (left->getMaterialTechniqueId() < right);
			}

			[[nodiscard]] inline bool operator()(RendererRuntime::MaterialTechniqueId left, const RendererRuntime::MaterialTechnique* right) const
			{
				return (left < right->getMaterialTechniqueId());
			}
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void MaterialResource::setParentMaterialResourceId(MaterialResourceId parentMaterialResourceId)
	{
		if (mParentMaterialResourceId != parentMaterialResourceId)
		{
			const MaterialResourceId materialResourceId = getId();

			// Destroy all material techniques
			destroyAllMaterialTechniques();

			// Unregister from previous parent material resource
			const MaterialResourceManager& materialResourceManager = getResourceManager<MaterialResourceManager>();
			if (isValid(mParentMaterialResourceId))
			{
				MaterialResource& parentMaterialResource = materialResourceManager.getById(mParentMaterialResourceId);
				SortedChildMaterialResourceIds::const_iterator iterator = std::lower_bound(parentMaterialResource.mSortedChildMaterialResourceIds.cbegin(), parentMaterialResource.mSortedChildMaterialResourceIds.cend(), materialResourceId, ::detail::OrderByMaterialResourceId());
				assert(iterator != parentMaterialResource.mSortedChildMaterialResourceIds.end() && *iterator == materialResourceId);
				parentMaterialResource.mSortedChildMaterialResourceIds.erase(iterator);
			}

			// Set new parent material resource ID
			mParentMaterialResourceId = parentMaterialResourceId;
			if (isValid(mParentMaterialResourceId))
			{
				// Register to new parent material resource
				MaterialResource& parentMaterialResource = materialResourceManager.getById(mParentMaterialResourceId);
				assert(parentMaterialResource.getLoadingState() == IResource::LoadingState::LOADED);
				SortedChildMaterialResourceIds::const_iterator iterator = std::lower_bound(parentMaterialResource.mSortedChildMaterialResourceIds.cbegin(), parentMaterialResource.mSortedChildMaterialResourceIds.cend(), materialResourceId, ::detail::OrderByMaterialResourceId());
				assert(iterator == parentMaterialResource.mSortedChildMaterialResourceIds.end() || *iterator != materialResourceId);
				parentMaterialResource.mSortedChildMaterialResourceIds.insert(iterator, materialResourceId);

				// Setup material resource
				setAssetId(parentMaterialResource.getAssetId());
				mMaterialProperties = parentMaterialResource.mMaterialProperties;
				for (MaterialTechnique* materialTechnique : parentMaterialResource.mSortedMaterialTechniqueVector)
				{
					mSortedMaterialTechniqueVector.push_back(new MaterialTechnique(materialTechnique->getMaterialTechniqueId(), *this, materialTechnique->getMaterialBlueprintResourceId()));
				}
			}
			else
			{
				// Don't touch the child material resources, but reset everything else
				mMaterialProperties.removeAllProperties();
			}
		}
	}

	MaterialTechnique* MaterialResource::getMaterialTechniqueById(MaterialTechniqueId materialTechniqueId) const
	{
		SortedMaterialTechniqueVector::const_iterator iterator = std::lower_bound(mSortedMaterialTechniqueVector.cbegin(), mSortedMaterialTechniqueVector.cend(), materialTechniqueId, ::detail::OrderByMaterialTechniqueId());
		return (iterator != mSortedMaterialTechniqueVector.end() && (*iterator)->getMaterialTechniqueId() == materialTechniqueId) ? *iterator : nullptr;
	}
	
	void MaterialResource::destroyAllMaterialTechniques()
	{
		for (MaterialTechnique* materialTechnique : mSortedMaterialTechniqueVector)
		{
			delete materialTechnique;
		}
		mSortedMaterialTechniqueVector.clear();
	}

	void MaterialResource::releaseTextures()
	{
		// TODO(co) Cleanup
		for (MaterialTechnique* materialTechnique : mSortedMaterialTechniqueVector)
		{
			materialTechnique->clearTextures();
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	MaterialResource::~MaterialResource()
	{
		// Sanity checks
		assert(isInvalid(mParentMaterialResourceId));
		assert(mSortedChildMaterialResourceIds.empty());
		assert(mSortedMaterialTechniqueVector.empty());
		assert(mMaterialProperties.getSortedPropertyVector().empty());
		assert(mAttachedRenderables.empty());

		// Avoid crash in case of failed sanity check
		while (!mAttachedRenderables.empty())
		{
			mAttachedRenderables[0]->unsetMaterialResourceId();
		}
	}

	MaterialResource& MaterialResource::operator=(MaterialResource&& materialResource)
	{
		// Call base implementation
		IResource::operator=(std::move(materialResource));

		// Swap data
		// -> Lucky us that we're usually not referencing by using raw-pointers, so a simple swap does the trick
		std::swap(mParentMaterialResourceId,	   materialResource.mParentMaterialResourceId);
		std::swap(mSortedChildMaterialResourceIds, materialResource.mSortedChildMaterialResourceIds);
		std::swap(mSortedMaterialTechniqueVector,  materialResource.mSortedMaterialTechniqueVector);
		std::swap(mMaterialProperties,			   materialResource.mMaterialProperties);
		std::swap(mAttachedRenderables,			   materialResource.mAttachedRenderables);

		// Done
		return *this;
	}

	void MaterialResource::deinitializeElement()
	{
		// Sanity check
		assert(mAttachedRenderables.empty());

		// Avoid crash in case of failed sanity check
		while (!mAttachedRenderables.empty())
		{
			mAttachedRenderables[0]->unsetMaterialResourceId();
		}

		// Unset parent material resource ID
		setParentMaterialResourceId(getInvalid<MaterialResourceId>());

		// Inform child material resources, if required
		if (!mSortedChildMaterialResourceIds.empty())
		{
			const MaterialResourceManager& materialResourceManager = static_cast<MaterialResourceManager&>(getResourceManager());
			while (!mSortedChildMaterialResourceIds.empty())
			{
				const MaterialResourceId materialResourceId = mSortedChildMaterialResourceIds.front();
				materialResourceManager.getById(materialResourceId).setParentMaterialResourceId(getInvalid<MaterialResourceId>());
			}
			mSortedChildMaterialResourceIds.clear();
		}

		// Cleanup
		destroyAllMaterialTechniques();
		mMaterialProperties.removeAllProperties();

		// Call base implementation
		IResource::deinitializeElement();
	}

	bool MaterialResource::setPropertyByIdInternal(MaterialPropertyId materialPropertyId, const MaterialPropertyValue& materialPropertyValue, MaterialProperty::Usage materialPropertyUsage, bool changeOverwrittenState)
	{
		// Call the base implementation
		MaterialProperty* materialProperty = mMaterialProperties.setPropertyById(materialPropertyId, materialPropertyValue, materialPropertyUsage, changeOverwrittenState);
		if (nullptr != materialProperty)
		{
			// Perform derived work, if required to do so
			switch (materialProperty->getUsage())
			{
				case MaterialProperty::Usage::SHADER_UNIFORM:
					for (MaterialTechnique* materialTechnique : mSortedMaterialTechniqueVector)
					{
						materialTechnique->scheduleForShaderUniformUpdate();
					}
					break;

				case MaterialProperty::Usage::SHADER_COMBINATION:
					// Handled by "RendererRuntime::MaterialProperties::setPropertyById()"
					break;

				case MaterialProperty::Usage::RASTERIZER_STATE:
				case MaterialProperty::Usage::DEPTH_STENCIL_STATE:
				case MaterialProperty::Usage::BLEND_STATE:
					// TODO(co) Optimization: The calculation of the FNV1a hash of "Renderer::SerializedGraphicsPipelineState" is pretty fast, but maybe it makes sense to schedule the calculation in case many material properties are changed in a row?
					for (MaterialTechnique* materialTechnique : mSortedMaterialTechniqueVector)
					{
						materialTechnique->calculateSerializedGraphicsPipelineStateHash();
					}
					break;

				case MaterialProperty::Usage::TEXTURE_REFERENCE:
					for (MaterialTechnique* materialTechnique : mSortedMaterialTechniqueVector)
					{
						materialTechnique->clearTextures();
					}
					break;

				case MaterialProperty::Usage::STATIC:
					// Initial cached material data gathering is performed inside "RendererRuntime::Renderable::setMaterialResourceId()"

					// Optional "RenderQueueIndex" (e.g. compositor materials usually don't need this property)
					if (RENDER_QUEUE_INDEX_PROPERTY_ID == materialPropertyId)
					{
						const int renderQueueIndex = materialProperty->getIntegerValue();

						// Sanity checks
						assert(renderQueueIndex >= 0);
						assert(renderQueueIndex <= 255);

						// Update the cached material data of all attached renderables
						for (Renderable* renderable : mAttachedRenderables)
						{
							renderable->mRenderQueueIndex = static_cast<uint8_t>(renderQueueIndex);

							// In here we don't care about the fact that one and the same renderable manager instance might
							// update cached renderables data. It's not performance critical in here and resolving this will
							// require additional logic which itself has an performance impact. So keep it simple.
							renderable->getRenderableManager().updateCachedRenderablesData();
						}
					}

					// Optional "CastShadows" (e.g. compositor materials usually don't need this property)
					else if (CAST_SHADOWS_PROPERTY_ID == materialPropertyId)
					{
						// Update the cached material data of all attached renderables
						const bool castShadows = materialProperty->getBooleanValue();
						for (Renderable* renderable : mAttachedRenderables)
						{
							renderable->mCastShadows = castShadows;

							// In here we don't care about the fact that one and the same renderable manager instance might
							// update cached renderables data. It's not performance critical in here and resolving this will
							// require additional logic which itself has an performance impact. So keep it simple.
							renderable->getRenderableManager().updateCachedRenderablesData();
						}
					}
					break;

				case MaterialProperty::Usage::UNKNOWN:
				case MaterialProperty::Usage::SAMPLER_STATE:
				case MaterialProperty::Usage::GLOBAL_REFERENCE:
				case MaterialProperty::Usage::UNKNOWN_REFERENCE:
				case MaterialProperty::Usage::PASS_REFERENCE:
				case MaterialProperty::Usage::MATERIAL_REFERENCE:
				case MaterialProperty::Usage::INSTANCE_REFERENCE:
				case MaterialProperty::Usage::GLOBAL_REFERENCE_FALLBACK:
				default:
					// Nothing here
					break;
			}

			// Inform child material resources, if required
			const MaterialResourceManager& materialResourceManager = static_cast<MaterialResourceManager&>(getResourceManager());
			for (MaterialResourceId materialResourceId : mSortedChildMaterialResourceIds)
			{
				materialResourceManager.getById(materialResourceId).setPropertyByIdInternal(materialPropertyId, materialPropertyValue, materialPropertyUsage, false);
			}

			// Material property change detected
			return true;
		}

		// No material property change detected
		return false;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
