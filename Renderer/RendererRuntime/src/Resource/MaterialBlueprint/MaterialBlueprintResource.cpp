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
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Resource/MaterialBlueprint/Listener/IMaterialBlueprintResourceListener.h"
#include "RendererRuntime/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Resource/ShaderPiece/ShaderPieceResourceManager.h"
#include "RendererRuntime/Resource/Detail/ResourceStreamer.h"
#include "RendererRuntime/Asset/AssetManager.h"
#include "RendererRuntime/IRendererRuntime.h"

#include <chrono>
#include <thread>


// Disable warnings
// TODO(co) See "RendererRuntime::MaterialBlueprintResource::MaterialBlueprintResource()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
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
		typedef std::vector<RendererRuntime::ShaderPropertyId> ShaderPropertyIds;


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Internal helper class to iterate through all shader combinations
		*/
		class ShaderCombinationIterator
		{
		public:
			explicit ShaderCombinationIterator(size_t reserveSize)
			{
				mNumberOfPropertyValuesByPropertyIndex.reserve(reserveSize);
				mCurrentCombination.reserve(reserveSize);
			}

			void clear()
			{
				mNumberOfPropertyValuesByPropertyIndex.clear();
				mCurrentCombination.clear();
			}

			void addBoolProperty()
			{
				addIntegerProperty(2);
			}

			void addIntegerProperty(uint32_t numberOfIntegerValues)
			{
				mNumberOfPropertyValuesByPropertyIndex.push_back(numberOfIntegerValues);
			}

			bool getCurrentCombinationBoolProperty(size_t index) const
			{
				return (getCurrentCombinationIntegerProperty(index) > 0);
			}

			uint32_t getCurrentCombinationIntegerProperty(size_t index) const
			{
				assert(index < mCurrentCombination.size());
				return mCurrentCombination[index];
			}

			void startIterate()
			{
				// Start with every property value set to zero
				mCurrentCombination.resize(mNumberOfPropertyValuesByPropertyIndex.size());
				memset(mCurrentCombination.data(), 0, sizeof(uint32_t) * mNumberOfPropertyValuesByPropertyIndex.size());
			}

			bool iterate()
			{
				// Just a sanity check, in case someone forgot to start iterating first
				assert(mCurrentCombination.size() == mNumberOfPropertyValuesByPropertyIndex.size());

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
		inline void setShaderPropertiesPropertyValue(const RendererRuntime::MaterialBlueprintResource& materialBlueprintResource, RendererRuntime::MaterialPropertyId materialPropertyId, const RendererRuntime::MaterialPropertyValue& materialPropertyValue, ShaderPropertyIds& shaderPropertyIds, ShaderCombinationIterator& shaderCombinationIterator)
		{
			switch (materialPropertyValue.getValueType())
			{
				case RendererRuntime::MaterialPropertyValue::ValueType::BOOLEAN:
					shaderPropertyIds.push_back(materialPropertyId);	// Shader property ID and material property ID are identical, so this is valid
					shaderCombinationIterator.addBoolProperty();
					break;

				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER:
					shaderPropertyIds.push_back(materialPropertyId);	// Shader property ID and material property ID are identical, so this is valid
					shaderCombinationIterator.addIntegerProperty(static_cast<uint32_t>(materialBlueprintResource.getMaximumIntegerValueOfShaderProperty(materialPropertyId)));
					break;

				case RendererRuntime::MaterialPropertyValue::ValueType::UNKNOWN:
				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_2:
				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_3:
				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_4:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_2:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3_3:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4_4:
				case RendererRuntime::MaterialPropertyValue::ValueType::FILL_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::CULL_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK:
				case RendererRuntime::MaterialPropertyValue::ValueType::STENCIL_OP:
				case RendererRuntime::MaterialPropertyValue::ValueType::COMPARISON_FUNC:
				case RendererRuntime::MaterialPropertyValue::ValueType::BLEND:
				case RendererRuntime::MaterialPropertyValue::ValueType::BLEND_OP:
				case RendererRuntime::MaterialPropertyValue::ValueType::FILTER_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID:
				case RendererRuntime::MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
				default:
					assert(false);	// TODO(co) Error handling
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
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const int32_t MaterialBlueprintResource::MANDATORY_SHADER_PROPERTY = std::numeric_limits<int32_t>::max();


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
		// TODO(co) Implement more efficient solution: We need to extend "Runtime::ResourceStreamer" to request emergency immediate processing of requested resources
		ResourceStreamer& resourceStreamer = getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getResourceStreamer();
		while (LoadingState::LOADED != getLoadingState())
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1ms);
			resourceStreamer.dispatch();
		}
	}

	void MaterialBlueprintResource::fillCommandBuffer(Renderer::CommandBuffer& commandBuffer)
	{
		// Set the used graphics root signature
		Renderer::Command::SetGraphicsRootSignature::create(commandBuffer, mRootSignaturePtr);

		// Bind pass buffer manager, if required
		if (nullptr != mPassBufferManager)
		{
			mPassBufferManager->fillCommandBuffer(commandBuffer);
		}

		// Set our sampler states
		if (!mSamplerStates.empty())
		{
			// Create sampler resource group, if needed
			if (nullptr == mSamplerStateGroup)
			{
				std::vector<Renderer::IResource*> resources;
				const size_t numberOfSamplerStates = mSamplerStates.size();
				resources.resize(numberOfSamplerStates);
				for (size_t i = 0; i < numberOfSamplerStates; ++i)
				{
					resources[i] = mSamplerStates[i].samplerStatePtr;
				}
				// TODO(co) All sampler states need to be inside the same resource group, this needs to be guaranteed by design
				mSamplerStateGroup = mRootSignaturePtr->createResourceGroup(mSamplerStates[0].rootParameterIndex, static_cast<uint32_t>(numberOfSamplerStates), resources.data());
				RENDERER_SET_RESOURCE_DEBUG_NAME(mSamplerStateGroup, "Material blueprint")
			}

			// Set resource group
			Renderer::Command::SetGraphicsResourceGroup::create(commandBuffer, mSamplerStates[0].rootParameterIndex, mSamplerStateGroup);
		}

		// It's valid if a material blueprint resource doesn't contain a material uniform buffer (usually the case for compositor material blueprint resources)
		if (nullptr != mMaterialBufferManager)
		{
			mMaterialBufferManager->resetLastBoundPool();
		}
	}

	void MaterialBlueprintResource::createPipelineStateCaches(bool mandatoryOnly)
	{
		// Material blueprint resource must be fully loaded, meaning also all referenced shader resources
		assert(LoadingState::LOADED == getLoadingState());

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
									// Error, can't resolve reference
									assert(false);	// TODO(co) Error handling
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
							// Unsupported shader combination material property value type
							assert(false);
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
				const Renderer::IPipelineStatePtr pipelineStatePtr = mPipelineStateCacheManager.getPipelineStateCacheByCombination(getUninitialized<uint32_t>(), shaderProperties, true);
				assert(nullptr != pipelineStatePtr);	// TODO(co) Decent error handling
			}
			while (shaderCombinationIterator.iterate());
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	MaterialBlueprintResource::MaterialBlueprintResource() :
		mPipelineStateCacheManager(*this),
		mPipelineState(Renderer::PipelineStateBuilder()),
		mVertexAttributesResourceId(getUninitialized<ShaderBlueprintResourceId>()),
		mPassUniformBuffer(nullptr),
		mMaterialUniformBuffer(nullptr),
		mInstanceUniformBuffer(nullptr),
		mInstanceTextureBuffer(nullptr),
		mLightTextureBuffer(nullptr),
		mPassBufferManager(nullptr),
		mMaterialBufferManager(nullptr)
	{
		memset(mShaderBlueprintResourceId, static_cast<int>(getUninitialized<ShaderBlueprintResourceId>()), sizeof(ShaderBlueprintResourceId) * NUMBER_OF_SHADER_TYPES);
	}

	MaterialBlueprintResource::~MaterialBlueprintResource()
	{
		// Destroy manager instances, if needed
		delete mPassBufferManager;
		delete mMaterialBufferManager;

		// TODO(co) Sanity checks
		/*
		PipelineStateCacheManager			 mPipelineStateCacheManager;
		MaterialProperties					 mMaterialProperties;
		ShaderProperties					 mVisualImportanceOfShaderProperties;	///< Every shader property known to the material blueprint has a visual importance entry in here
		ShaderProperties					 mMaximumIntegerValueOfShaderProperties;
		Renderer::VertexAttributes			 mVertexAttributes;
		Renderer::IRootSignaturePtr			 mRootSignaturePtr;						///< Root signature, can be a null pointer
		Renderer::PipelineState				 mPipelineState;
		VertexAttributesResourceId			 mVertexAttributesResourceId;
		ShaderBlueprintResourceId			 mShaderBlueprintResourceId[NUMBER_OF_SHADER_TYPES];
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

	void MaterialBlueprintResource::onDefaultTextureFilteringChanged(Renderer::FilterMode defaultFilterMode, uint8_t maximumDefaultAnisotropy)
	{
		const IRendererRuntime& rendererRuntime = getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime();
		Renderer::IRenderer& renderer = rendererRuntime.getRenderer();
		const Asset* asset = rendererRuntime.getAssetManager().tryGetAssetByAssetId(getAssetId());
		if (nullptr != asset)
		{
			for (SamplerState& samplerState : mSamplerStates)
			{
				if (Renderer::FilterMode::UNKNOWN == samplerState.rendererSamplerState.filter || isUninitialized(samplerState.rendererSamplerState.maxAnisotropy))
				{
					mSamplerStateGroup = nullptr;
					Renderer::SamplerState rendererSamplerState = samplerState.rendererSamplerState;
					if (Renderer::FilterMode::UNKNOWN == rendererSamplerState.filter)
					{
						rendererSamplerState.filter = defaultFilterMode;
					}
					if (isUninitialized(rendererSamplerState.maxAnisotropy))
					{
						rendererSamplerState.maxAnisotropy = maximumDefaultAnisotropy;
					}
					samplerState.samplerStatePtr = renderer.createSamplerState(rendererSamplerState);
					RENDERER_SET_RESOURCE_DEBUG_NAME(samplerState.samplerStatePtr, asset->virtualFilename)
				}
			}
		}
	}

	void MaterialBlueprintResource::clearPipelineStateObjectCache()
	{
		// TODO(co) Implement me
		NOP;
	}

	void MaterialBlueprintResource::loadPipelineStateObjectCache(IFile&)
	{
		// TODO(co) Implement me
		NOP;
	}

	bool MaterialBlueprintResource::doesPipelineStateObjectCacheNeedSaving() const
	{
		// TODO(co) Implement me
		return false;
	}

	void MaterialBlueprintResource::savePipelineStateObjectCache(IFile&)
	{
		// TODO(co) Implement me
		NOP;
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
		setUninitialized(mVertexAttributesResourceId);
		memset(mShaderBlueprintResourceId, static_cast<int>(getUninitialized<ShaderBlueprintResourceId>()), sizeof(ShaderBlueprintResourceId) * NUMBER_OF_SHADER_TYPES);

		// Call base implementation
		IResource::deinitializeElement();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
