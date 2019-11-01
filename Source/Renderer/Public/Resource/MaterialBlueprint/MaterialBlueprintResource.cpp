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
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/Listener/IMaterialBlueprintResourceListener.h"
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "Renderer/Public/Resource/ShaderPiece/ShaderPieceResourceManager.h"
#include "Renderer/Public/Resource/ResourceStreamer.h"
#include "Renderer/Public/Asset/AssetManager.h"
#include "Renderer/Public/IRenderer.h"

#include <chrono>
#include <thread>


// Disable warnings
// TODO(co) See "Renderer::MaterialBlueprintResource::MaterialBlueprintResource()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		typedef std::vector<Renderer::ShaderPropertyId> ShaderPropertyIds;


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Internal helper class to iterate through all shader combinations
		*/
		class ShaderCombinationIterator final
		{
		public:
			inline explicit ShaderCombinationIterator(size_t reserveSize)
			{
				mNumberOfPropertyValuesByPropertyIndex.reserve(reserveSize);
				mCurrentCombination.reserve(reserveSize);
			}

			inline void clear()
			{
				mNumberOfPropertyValuesByPropertyIndex.clear();
				mCurrentCombination.clear();
			}

			inline void addBoolProperty()
			{
				addIntegerProperty(2);
			}

			inline void addIntegerProperty(uint32_t numberOfIntegerValues)
			{
				mNumberOfPropertyValuesByPropertyIndex.push_back(numberOfIntegerValues);
			}

			[[nodiscard]] inline bool getCurrentCombinationBoolProperty(size_t index) const
			{
				return (getCurrentCombinationIntegerProperty(index) > 0);
			}

			[[nodiscard]] inline uint32_t getCurrentCombinationIntegerProperty(size_t index) const
			{
				ASSERT(index < mCurrentCombination.size());
				return mCurrentCombination[index];
			}

			inline void startIterate()
			{
				// Start with every property value set to zero
				mCurrentCombination.resize(mNumberOfPropertyValuesByPropertyIndex.size());
				memset(mCurrentCombination.data(), 0, sizeof(uint32_t) * mNumberOfPropertyValuesByPropertyIndex.size());
			}

			[[nodiscard]] bool iterate()
			{
				// Just a sanity check, in case someone forgot to start iterating first
				ASSERT(mCurrentCombination.size() == mNumberOfPropertyValuesByPropertyIndex.size());

				for (size_t index = 0; index < mCurrentCombination.size(); ++index)
				{
					uint32_t& propertyValue = mCurrentCombination[index];
					++propertyValue;
					if (propertyValue < mNumberOfPropertyValuesByPropertyIndex[index])
					{
						// Went up by one, result is valid, so everything is fine
						return true;
					}
					else
					{
						// We have to go to the next property now and increase that one; but first reset this one here to zero again
						propertyValue = 0;
					}
				}

				// We're done with iterating, every property is at its maximum
				return false;
			}
		private:
			std::vector<uint32_t> mNumberOfPropertyValuesByPropertyIndex;
			std::vector<uint32_t> mCurrentCombination;
		};


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		inline void setShaderPropertiesPropertyValue(const Renderer::MaterialBlueprintResource& materialBlueprintResource, Renderer::MaterialPropertyId materialPropertyId, const Renderer::MaterialPropertyValue& materialPropertyValue, ShaderPropertyIds& shaderPropertyIds, ShaderCombinationIterator& shaderCombinationIterator)
		{
			switch (materialPropertyValue.getValueType())
			{
				case Renderer::MaterialPropertyValue::ValueType::BOOLEAN:
					shaderPropertyIds.push_back(materialPropertyId);	// Shader property ID and material property ID are identical, so this is valid
					shaderCombinationIterator.addBoolProperty();
					break;

				case Renderer::MaterialPropertyValue::ValueType::INTEGER:
					shaderPropertyIds.push_back(materialPropertyId);	// Shader property ID and material property ID are identical, so this is valid
					shaderCombinationIterator.addIntegerProperty(static_cast<uint32_t>(materialBlueprintResource.getMaximumIntegerValueOfShaderProperty(materialPropertyId)));
					break;

				case Renderer::MaterialPropertyValue::ValueType::UNKNOWN:
				case Renderer::MaterialPropertyValue::ValueType::INTEGER_2:
				case Renderer::MaterialPropertyValue::ValueType::INTEGER_3:
				case Renderer::MaterialPropertyValue::ValueType::INTEGER_4:
				case Renderer::MaterialPropertyValue::ValueType::FLOAT:
				case Renderer::MaterialPropertyValue::ValueType::FLOAT_2:
				case Renderer::MaterialPropertyValue::ValueType::FLOAT_3:
				case Renderer::MaterialPropertyValue::ValueType::FLOAT_4:
				case Renderer::MaterialPropertyValue::ValueType::FLOAT_3_3:
				case Renderer::MaterialPropertyValue::ValueType::FLOAT_4_4:
				case Renderer::MaterialPropertyValue::ValueType::FILL_MODE:
				case Renderer::MaterialPropertyValue::ValueType::CULL_MODE:
				case Renderer::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
				case Renderer::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK:
				case Renderer::MaterialPropertyValue::ValueType::STENCIL_OP:
				case Renderer::MaterialPropertyValue::ValueType::COMPARISON_FUNC:
				case Renderer::MaterialPropertyValue::ValueType::BLEND:
				case Renderer::MaterialPropertyValue::ValueType::BLEND_OP:
				case Renderer::MaterialPropertyValue::ValueType::FILTER_MODE:
				case Renderer::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE:
				case Renderer::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID:
				case Renderer::MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
				default:
					ASSERT(false);	// TODO(co) Error handling
					break;
			}
		}


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
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	MaterialProperty::Usage MaterialBlueprintResource::getMaterialPropertyUsageFromBufferUsage(BufferUsage bufferUsage)
	{
		switch (bufferUsage)
		{
			case BufferUsage::UNKNOWN:
				return MaterialProperty::Usage::UNKNOWN_REFERENCE;

			case BufferUsage::PASS:
				return MaterialProperty::Usage::PASS_REFERENCE;

			case BufferUsage::MATERIAL:
				return MaterialProperty::Usage::MATERIAL_REFERENCE;

			case BufferUsage::INSTANCE:
				return MaterialProperty::Usage::INSTANCE_REFERENCE;

			case BufferUsage::LIGHT:
				return MaterialProperty::Usage::UNKNOWN_REFERENCE;
		}

		// Error, we should never ever end up in here
		return MaterialProperty::Usage::UNKNOWN_REFERENCE;
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void MaterialBlueprintResource::optimizeShaderProperties(const ShaderProperties& shaderProperties, ShaderProperties& optimizedShaderProperties) const
	{
		// Gather relevant shader properties
		optimizedShaderProperties.clear();
		for (const ShaderProperties::Property& property : shaderProperties.getSortedPropertyVector())
		{
			if (0 != property.value && mVisualImportanceOfShaderProperties.hasPropertyValue(property.shaderPropertyId))
			{
				optimizedShaderProperties.setPropertyValue(property.shaderPropertyId, property.value);
			}
		}
	}

	void MaterialBlueprintResource::enforceFullyLoaded()
	{
		// TODO(co) Implement more efficient solution: We need to extend "::ResourceStreamer" to request emergency immediate processing of requested resources
		ResourceStreamer& resourceStreamer = getResourceManager<MaterialBlueprintResourceManager>().getRenderer().getResourceStreamer();
		while (LoadingState::LOADED != getLoadingState())
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1ms);
			resourceStreamer.dispatch();
		}
	}

	void MaterialBlueprintResource::fillGraphicsCommandBuffer(Rhi::CommandBuffer& commandBuffer)
	{
		// Set the used graphics root signature
		Rhi::Command::SetGraphicsRootSignature::create(commandBuffer, mRootSignaturePtr);

		// Bind pass buffer manager, if required
		if (nullptr != mPassBufferManager)
		{
			mPassBufferManager->fillGraphicsCommandBuffer(commandBuffer);
		}

		// Set our sampler states
		if (!mSamplerStates.empty())
		{
			// Create sampler resource group, if needed
			if (nullptr == mSamplerStateGroup)
			{
				std::vector<Rhi::IResource*> resources;
				const size_t numberOfSamplerStates = mSamplerStates.size();
				resources.resize(numberOfSamplerStates);
				for (size_t i = 0; i < numberOfSamplerStates; ++i)
				{
					resources[i] = mSamplerStates[i].samplerStatePtr;
				}
				// TODO(co) All sampler states need to be inside the same resource group, this needs to be guaranteed by design
				mSamplerStateGroup = mRootSignaturePtr->createResourceGroup(mSamplerStates[0].rootParameterIndex, static_cast<uint32_t>(numberOfSamplerStates), resources.data());
				RHI_SET_RESOURCE_DEBUG_NAME(mSamplerStateGroup, "Material blueprint")
			}

			// Set graphics resource group
			Rhi::Command::SetGraphicsResourceGroup::create(commandBuffer, mSamplerStates[0].rootParameterIndex, mSamplerStateGroup);
		}

		// It's valid if a graphics material blueprint resource doesn't contain a material uniform buffer (usually the case for compositor material blueprint resources)
		if (nullptr != mMaterialBufferManager)
		{
			mMaterialBufferManager->resetLastGraphicsBoundPool();
		}
	}

	void MaterialBlueprintResource::fillComputeCommandBuffer(Rhi::CommandBuffer& commandBuffer)
	{
		// Set the used compute root signature
		Rhi::Command::SetComputeRootSignature::create(commandBuffer, mRootSignaturePtr);

		// Bind pass buffer manager, if required
		if (nullptr != mPassBufferManager)
		{
			mPassBufferManager->fillComputeCommandBuffer(commandBuffer);
		}

		// Set our sampler states
		if (!mSamplerStates.empty())
		{
			// Create sampler resource group, if needed
			if (nullptr == mSamplerStateGroup)
			{
				std::vector<Rhi::IResource*> resources;
				const size_t numberOfSamplerStates = mSamplerStates.size();
				resources.resize(numberOfSamplerStates);
				for (size_t i = 0; i < numberOfSamplerStates; ++i)
				{
					resources[i] = mSamplerStates[i].samplerStatePtr;
				}
				// TODO(co) All sampler states need to be inside the same resource group, this needs to be guaranteed by design
				mSamplerStateGroup = mRootSignaturePtr->createResourceGroup(mSamplerStates[0].rootParameterIndex, static_cast<uint32_t>(numberOfSamplerStates), resources.data());
				RHI_SET_RESOURCE_DEBUG_NAME(mSamplerStateGroup, "Material blueprint")
			}

			// Set compute resource group
			Rhi::Command::SetComputeResourceGroup::create(commandBuffer, mSamplerStates[0].rootParameterIndex, mSamplerStateGroup);
		}

		// It's valid if a compute material blueprint resource doesn't contain a material uniform buffer (usually the case for compositor material blueprint resources)
		if (nullptr != mMaterialBufferManager)
		{
			mMaterialBufferManager->resetLastComputeBoundPool();
		}
	}

	void MaterialBlueprintResource::createPipelineStateCaches(bool mandatoryOnly)
	{
		// Sanity check
		RHI_ASSERT(getResourceManager<MaterialBlueprintResourceManager>().getRenderer().getContext(), LoadingState::LOADED == getLoadingState(), "Material blueprint resource must be fully loaded, meaning also all referenced shader resources")

		// TODO(co) Optimization: Avoid constant allocations/deallocations, can't use a static instance to not get false-positive memory-leaks, add maybe some kind of context?
		::detail::ShaderCombinationIterator shaderCombinationIterator(128);
		ShaderProperties shaderProperties(128);
		::detail::ShaderPropertyIds shaderPropertyIds;
		shaderPropertyIds.reserve(128);

		{ // Gather all mandatory shader combination properties
			const MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector = mMaterialProperties.getSortedPropertyVector();
			for (const MaterialProperty& materialProperty : sortedMaterialPropertyVector)
			{
				const MaterialPropertyId materialPropertyId = materialProperty.getMaterialPropertyId();
				if (materialProperty.getUsage() == MaterialProperty::Usage::SHADER_COMBINATION && (!mandatoryOnly || mVisualImportanceOfShaderProperties.getPropertyValueUnsafe(materialPropertyId) == MANDATORY_SHADER_PROPERTY))
				{
					switch (materialProperty.getValueType())
					{
						case MaterialProperty::ValueType::BOOLEAN:
							shaderPropertyIds.push_back(materialProperty.getMaterialPropertyId());	// Shader property ID and material property ID are identical, so this is valid
							shaderCombinationIterator.addBoolProperty();
							break;

						case MaterialProperty::ValueType::INTEGER:
							shaderPropertyIds.push_back(materialProperty.getMaterialPropertyId());	// Shader property ID and material property ID are identical, so this is valid
							shaderCombinationIterator.addIntegerProperty(static_cast<uint32_t>(getMaximumIntegerValueOfShaderProperty(materialPropertyId)));
							break;

						case MaterialProperty::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
						{
							const MaterialProperty* globalMaterialProperty = getResourceManager<MaterialBlueprintResourceManager>().getGlobalMaterialProperties().getPropertyById(materialProperty.getGlobalMaterialPropertyId());
							if (nullptr != globalMaterialProperty)
							{
								::detail::setShaderPropertiesPropertyValue(*this, materialProperty.getMaterialPropertyId(), *globalMaterialProperty, shaderPropertyIds, shaderCombinationIterator);
							}
							else
							{
								// Try global material property reference fallback
								globalMaterialProperty = mMaterialProperties.getPropertyById(materialProperty.getGlobalMaterialPropertyId());
								if (nullptr != globalMaterialProperty)
								{
									::detail::setShaderPropertiesPropertyValue(*this, materialProperty.getMaterialPropertyId(), *globalMaterialProperty, shaderPropertyIds, shaderCombinationIterator);
								}
								else
								{
									// Error!
									RHI_ASSERT(getResourceManager<MaterialBlueprintResourceManager>().getRenderer().getContext(), false, "Can't resolve reference")
								}
							}
							break;
						}

						case MaterialProperty::ValueType::UNKNOWN:
						case MaterialProperty::ValueType::INTEGER_2:
						case MaterialProperty::ValueType::INTEGER_3:
						case MaterialProperty::ValueType::INTEGER_4:
						case MaterialProperty::ValueType::FLOAT:
						case MaterialProperty::ValueType::FLOAT_2:
						case MaterialProperty::ValueType::FLOAT_3:
						case MaterialProperty::ValueType::FLOAT_4:
						case MaterialProperty::ValueType::FLOAT_3_3:
						case MaterialProperty::ValueType::FLOAT_4_4:
						case MaterialProperty::ValueType::FILL_MODE:
						case MaterialProperty::ValueType::CULL_MODE:
						case MaterialProperty::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
						case MaterialProperty::ValueType::DEPTH_WRITE_MASK:
						case MaterialProperty::ValueType::STENCIL_OP:
						case MaterialProperty::ValueType::COMPARISON_FUNC:
						case MaterialProperty::ValueType::BLEND:
						case MaterialProperty::ValueType::BLEND_OP:
						case MaterialProperty::ValueType::FILTER_MODE:
						case MaterialProperty::ValueType::TEXTURE_ADDRESS_MODE:
						case MaterialProperty::ValueType::TEXTURE_ASSET_ID:
						default:
							// Error!
							RHI_ASSERT(getResourceManager<MaterialBlueprintResourceManager>().getRenderer().getContext(), false, "Unsupported shader combination material property value type")
							break;
					}
				}
			}
		}

		{ // Create the pipeline state caches
			const uint32_t numberOfShaderProperties = static_cast<uint32_t>(shaderPropertyIds.size());
			shaderCombinationIterator.startIterate();
			do
			{
				// Set the current shader properties combination
				// -> The value always starts with 0 and has no holes in enumeration
				shaderProperties.clear();
				for (uint32_t i = 0; i < numberOfShaderProperties; ++i)
				{
					const uint32_t value = shaderCombinationIterator.getCurrentCombinationIntegerProperty(i);
					if (value != 0)
					{
						shaderProperties.setPropertyValue(shaderPropertyIds[i], static_cast<int32_t>(value));
					}
				}

				// Create the current pipeline state cache instances for the material blueprint
				if (isValid(mComputeShaderBlueprintResourceId))
				{
					[[maybe_unused]] const ComputePipelineStateCache* computePipelineStateCache = mComputePipelineStateCacheManager.getComputePipelineStateCacheByCombination(shaderProperties, false);
				}
				else
				{
					[[maybe_unused]] const GraphicsPipelineStateCache* graphicsPipelineStateCache = mGraphicsPipelineStateCacheManager.getGraphicsPipelineStateCacheByCombination(getInvalid<uint32_t>(), shaderProperties, false);
				}
			}
			while (shaderCombinationIterator.iterate());
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	MaterialBlueprintResource::MaterialBlueprintResource() :
		mGraphicsPipelineStateCacheManager(*this),
		mComputePipelineStateCacheManager(*this),
		mGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder()),
		mVertexAttributesResourceId(getInvalid<VertexAttributesResourceId>()),
		mGraphicsShaderBlueprintResourceId{getInvalid<ShaderBlueprintResourceId>(), getInvalid<ShaderBlueprintResourceId>(), getInvalid<ShaderBlueprintResourceId>(), getInvalid<ShaderBlueprintResourceId>(), getInvalid<ShaderBlueprintResourceId>()},
		mComputeShaderBlueprintResourceId(getInvalid<ShaderBlueprintResourceId>()),
		mPassUniformBuffer(nullptr),
		mMaterialUniformBuffer(nullptr),
		mInstanceUniformBuffer(nullptr),
		mInstanceTextureBuffer(nullptr),
		mLightTextureBuffer(nullptr),
		mPassBufferManager(nullptr),
		mMaterialBufferManager(nullptr)
	{
		// Nothing here
	}

	MaterialBlueprintResource::~MaterialBlueprintResource()
	{
		// Destroy manager instances, if needed
		delete mPassBufferManager;
		delete mMaterialBufferManager;

		// TODO(co) Sanity checks
		/*
		GraphicsPipelineStateCacheManager mGraphicsPipelineStateCacheManager;
		ComputePipelineStateCacheManager  mComputePipelineStateCacheManager;
		MaterialProperties				  mMaterialProperties;
		ShaderProperties				  mVisualImportanceOfShaderProperties;	///< Every shader property known to the material blueprint has a visual importance entry in here
		ShaderProperties				  mMaximumIntegerValueOfShaderProperties;
		// Graphics pipeline state
		Rhi::GraphicsPipelineState		mGraphicsPipelineState;
		VertexAttributesResourceId		mVertexAttributesResourceId;
		ShaderBlueprintResourceId		mGraphicsShaderBlueprintResourceId[NUMBER_OF_GRAPHICS_SHADER_TYPES];
		// Compute pipeline state
		ShaderBlueprintResourceId		mComputeShaderBlueprintResourceId;
		// Resource
		UniformBuffers mUniformBuffers;
		SamplerStates  mSamplerStates;
		Textures	   mTextures;
		// Ease-of-use direct access
		UniformBuffer* mPassUniformBuffer;		///< Can be a null pointer, don't destroy the instance
		UniformBuffer* mMaterialUniformBuffer;	///< Can be a null pointer, don't destroy the instance
		UniformBuffer* mInstanceUniformBuffer;	///< Can be a null pointer, don't destroy the instance
		TextureBuffer* mInstanceTextureBuffer;	///< Can be a null pointer, don't destroy the instance
		TextureBuffer* mLightTextureBuffer;		///< Can be a null pointer, don't destroy the instance
		*/
	}

	void MaterialBlueprintResource::onDefaultTextureFilteringChanged(Rhi::FilterMode defaultFilterMode, uint8_t maximumDefaultAnisotropy)
	{
		const IRenderer& renderer = getResourceManager<MaterialBlueprintResourceManager>().getRenderer();
		Rhi::IRhi& rhi = renderer.getRhi();
		const Asset* asset = renderer.getAssetManager().tryGetAssetByAssetId(getAssetId());
		if (nullptr != asset)
		{
			for (SamplerState& samplerState : mSamplerStates)
			{
				if (Rhi::FilterMode::UNKNOWN == samplerState.rhiSamplerState.filter || isInvalid(samplerState.rhiSamplerState.maxAnisotropy))
				{
					mSamplerStateGroup = nullptr;
					Rhi::SamplerState rhiSamplerState = samplerState.rhiSamplerState;
					if (Rhi::FilterMode::UNKNOWN == rhiSamplerState.filter)
					{
						rhiSamplerState.filter = defaultFilterMode;
					}
					if (isInvalid(rhiSamplerState.maxAnisotropy))
					{
						rhiSamplerState.maxAnisotropy = maximumDefaultAnisotropy;
					}
					samplerState.samplerStatePtr = rhi.createSamplerState(rhiSamplerState);
					RHI_SET_RESOURCE_DEBUG_NAME(samplerState.samplerStatePtr, asset->virtualFilename)
				}
			}
		}
	}

	void MaterialBlueprintResource::clearPipelineStateObjectCache()
	{
		mGraphicsPipelineStateCacheManager.clearCache();
		mComputePipelineStateCacheManager.clearCache();
	}

	void MaterialBlueprintResource::loadPipelineStateObjectCache(IFile& file)
	{
		mGraphicsPipelineStateCacheManager.loadPipelineStateObjectCache(file);
		mComputePipelineStateCacheManager.loadPipelineStateObjectCache(file);
	}

	bool MaterialBlueprintResource::doesPipelineStateObjectCacheNeedSaving() const
	{
		return (mGraphicsPipelineStateCacheManager.doesPipelineStateObjectCacheNeedSaving() || mComputePipelineStateCacheManager.doesPipelineStateObjectCacheNeedSaving());
	}

	void MaterialBlueprintResource::savePipelineStateObjectCache(IFile& file)
	{
		mGraphicsPipelineStateCacheManager.savePipelineStateObjectCache(file);
		mComputePipelineStateCacheManager.savePipelineStateObjectCache(file);
	}

	void MaterialBlueprintResource::initializeElement(MaterialBlueprintResourceId materialBlueprintResourceId)
	{
		// TODO(co) Sanity checks

		// Call base implementation
		IResource::initializeElement(materialBlueprintResourceId);
	}

	void MaterialBlueprintResource::deinitializeElement()
	{
		// TODO(co) Reset everything
		setInvalid(mVertexAttributesResourceId);
		memset(mGraphicsShaderBlueprintResourceId, static_cast<int>(getInvalid<ShaderBlueprintResourceId>()), sizeof(ShaderBlueprintResourceId) * NUMBER_OF_GRAPHICS_SHADER_TYPES);
		setInvalid(mComputeShaderBlueprintResourceId);

		// Call base implementation
		IResource::deinitializeElement();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
