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
#include "RendererRuntime/Public/Resource/Material/MaterialTechnique.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResource.h"
#include "RendererRuntime/Public/Resource/RendererResourceManager.h"
#include "RendererRuntime/Public/Core/Math/Math.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


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
		#define DEFINE_CONSTANT(name) static constexpr uint32_t name = STRING_ID(#name);
			DEFINE_CONSTANT(CullMode)
			DEFINE_CONSTANT(AlphaToCoverageEnable)
		#undef DEFINE_CONSTANT


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
	MaterialTechnique::MaterialTechnique(MaterialTechniqueId materialTechniqueId, MaterialResource& materialResource, MaterialBlueprintResourceId materialBlueprintResourceId) :
		MaterialBufferSlot(materialResource),
		mMaterialTechniqueId(materialTechniqueId),
		mMaterialBlueprintResourceId(materialBlueprintResourceId),
		mSerializedGraphicsPipelineStateHash(getInvalid<uint32_t>())
	{
		MaterialBufferManager* materialBufferManager = getMaterialBufferManager();
		if (nullptr != materialBufferManager)
		{
			materialBufferManager->requestSlot(*this);
		}

		// Calculate FNV1a hash of "Renderer::SerializedGraphicsPipelineState"
		calculateSerializedGraphicsPipelineStateHash();
	}

	MaterialTechnique::~MaterialTechnique()
	{
		// Due to hot-reloading it's possible that there's no assigned material slot, so we need to do a check here
		if (isValid(getAssignedMaterialSlot()))
		{
			MaterialBufferManager* materialBufferManager = getMaterialBufferManager();
			assert(nullptr != materialBufferManager);
			materialBufferManager->releaseSlot(*this);
		}
	}

	const MaterialTechnique::Textures& MaterialTechnique::getTextures(const IRendererRuntime& rendererRuntime)
	{
		// Need for gathering the textures now?
		if (mTextures.empty())
		{
			const MaterialBlueprintResource* materialBlueprintResource = rendererRuntime.getMaterialBlueprintResourceManager().tryGetById(mMaterialBlueprintResourceId);
			if (nullptr != materialBlueprintResource)
			{
				const MaterialResource& materialResource = getMaterialResource();
				TextureResourceManager& textureResourceManager = rendererRuntime.getTextureResourceManager();
				const MaterialBlueprintResource::Textures& textures = materialBlueprintResource->getTextures();
				const size_t numberOfTextures = textures.size();
				for (size_t i = 0; i < numberOfTextures; ++i)
				{
					const MaterialBlueprintResource::Texture& blueprintTexture = textures[i];

					// Start with the material blueprint textures
					Texture texture;
					texture.rootParameterIndex = blueprintTexture.rootParameterIndex;
					texture.materialProperty   = blueprintTexture.materialProperty;
					texture.textureResourceId  = blueprintTexture.textureResourceId;

					// Apply material specific modifications
					const MaterialPropertyId materialPropertyId = texture.materialProperty.getMaterialPropertyId();
					if (isValid(materialPropertyId))
					{
						// Figure out the material property value
						const MaterialProperty* materialProperty = materialResource.getPropertyById(materialPropertyId);
						if (nullptr != materialProperty)
						{
							// TODO(co) Error handling: Usage mismatch etc.
							texture.materialProperty = *materialProperty;
							textureResourceManager.loadTextureResourceByAssetId(texture.materialProperty.getTextureAssetIdValue(), blueprintTexture.fallbackTextureAssetId, texture.textureResourceId, this, blueprintTexture.rgbHardwareGammaCorrection);
						}
					}

					// Insert texture
					mTextures.push_back(texture);
				}
			}
		}
		return mTextures;
	}

	void MaterialTechnique::fillGraphicsCommandBuffer(const IRendererRuntime& rendererRuntime, Renderer::CommandBuffer& commandBuffer, uint32_t& textureResourceGroupRootParameterIndex, Renderer::IResourceGroup** textureResourceGroup)
	{
		// Sanity check
		assert(isValid(mMaterialBlueprintResourceId));
		assert((nullptr != textureResourceGroup) && "The renderer texture resource group pointer must be valid");

		{ // Bind the material buffer manager
			MaterialBufferManager* materialBufferManager = getMaterialBufferManager();
			if (nullptr != materialBufferManager)
			{
				materialBufferManager->fillGraphicsCommandBuffer(*this, commandBuffer);
			}
		}

		// Set textures
		fillCommandBuffer(rendererRuntime, textureResourceGroupRootParameterIndex, textureResourceGroup);
	}

	void MaterialTechnique::fillComputeCommandBuffer(const IRendererRuntime& rendererRuntime, Renderer::CommandBuffer& commandBuffer, uint32_t& textureResourceGroupRootParameterIndex, Renderer::IResourceGroup** textureResourceGroup)
	{
		// Sanity check
		assert(isValid(mMaterialBlueprintResourceId));
		assert((nullptr != textureResourceGroup) && "The renderer texture resource group pointer must be valid");

		{ // Bind the material buffer manager
			MaterialBufferManager* materialBufferManager = getMaterialBufferManager();
			if (nullptr != materialBufferManager)
			{
				materialBufferManager->fillComputeCommandBuffer(*this, commandBuffer);
			}
		}

		// Set textures
		fillCommandBuffer(rendererRuntime, textureResourceGroupRootParameterIndex, textureResourceGroup);
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	void MaterialTechnique::onLoadingStateChange(const RendererRuntime::IResource&)
	{
		makeTextureResourceGroupDirty();
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	MaterialBufferManager* MaterialTechnique::getMaterialBufferManager() const
	{
		// It's valid if a material blueprint resource doesn't contain a material uniform buffer (usually the case for compositor material blueprint resources)
		const MaterialBlueprintResource* materialBlueprintResource = getMaterialResourceManager().getRendererRuntime().getMaterialBlueprintResourceManager().tryGetById(mMaterialBlueprintResourceId);
		return (nullptr != materialBlueprintResource) ? materialBlueprintResource->getMaterialBufferManager() : nullptr;
	}

	void MaterialTechnique::calculateSerializedGraphicsPipelineStateHash()
	{
		const MaterialBlueprintResource* materialBlueprintResource = getMaterialResourceManager().getRendererRuntime().getMaterialBlueprintResourceManager().tryGetById(mMaterialBlueprintResourceId);
		if (nullptr != materialBlueprintResource)
		{
			// Start with the graphics pipeline state of the material blueprint resource
			Renderer::SerializedGraphicsPipelineState serializedGraphicsPipelineState = materialBlueprintResource->getGraphicsPipelineState();

			// Apply material properties
			// -> Renderer toolkit counterpart is "RendererToolkit::JsonMaterialBlueprintHelper::readPipelineStateObject()"
			const MaterialProperties& materialBlueprintMaterialProperties = materialBlueprintResource->getMaterialProperties();
			const MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector = getMaterialResource().getSortedPropertyVector();
			const size_t numberOfMaterialProperties = sortedMaterialPropertyVector.size();
			for (size_t i = 0; i < numberOfMaterialProperties; ++i)
			{
				// A material can have multiple material techniques, do only apply material properties which are known to the material blueprint resource
				const MaterialProperty& materialProperty = sortedMaterialPropertyVector[i];
				if (nullptr != materialBlueprintMaterialProperties.getPropertyById(materialProperty.getMaterialPropertyId()))
				{
					switch (materialProperty.getUsage())
					{
						case MaterialProperty::Usage::UNKNOWN:
						case MaterialProperty::Usage::STATIC:
						case MaterialProperty::Usage::SHADER_UNIFORM:
						case MaterialProperty::Usage::SHADER_COMBINATION:
							// Nothing here
							break;

						case MaterialProperty::Usage::RASTERIZER_STATE:
							// TODO(co) Implement all rasterizer state properties
							if (materialProperty.getMaterialPropertyId() == ::detail::CullMode)
							{
								serializedGraphicsPipelineState.rasterizerState.cullMode = materialProperty.getCullModeValue();
							}
							break;

						case MaterialProperty::Usage::DEPTH_STENCIL_STATE:
							// TODO(co) Implement all depth stencil state properties
							break;

						case MaterialProperty::Usage::BLEND_STATE:
							// TODO(co) Implement all blend state properties
							if (materialProperty.getMaterialPropertyId() == ::detail::AlphaToCoverageEnable)
							{
								serializedGraphicsPipelineState.blendState.alphaToCoverageEnable = materialProperty.getBooleanValue();
							}
							break;

						case MaterialProperty::Usage::SAMPLER_STATE:
						case MaterialProperty::Usage::TEXTURE_REFERENCE:
						case MaterialProperty::Usage::GLOBAL_REFERENCE:
						case MaterialProperty::Usage::UNKNOWN_REFERENCE:
						case MaterialProperty::Usage::PASS_REFERENCE:
						case MaterialProperty::Usage::MATERIAL_REFERENCE:
						case MaterialProperty::Usage::INSTANCE_REFERENCE:
						case MaterialProperty::Usage::GLOBAL_REFERENCE_FALLBACK:
							// Nothing here
							break;
					}
				}
			}

			// Calculate the FNV1a hash of "Renderer::SerializedGraphicsPipelineState"
			mSerializedGraphicsPipelineStateHash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&serializedGraphicsPipelineState), sizeof(Renderer::SerializedGraphicsPipelineState));

			// Register the FNV1a hash of "Renderer::SerializedGraphicsPipelineState" inside the material blueprint resource manager so it's sufficient to pass around the tiny hash instead the over 400 bytes full serialized pipeline state
			getMaterialResourceManager().getRendererRuntime().getMaterialBlueprintResourceManager().addSerializedGraphicsPipelineState(mSerializedGraphicsPipelineStateHash, serializedGraphicsPipelineState);
		}
		else
		{
			setInvalid(mSerializedGraphicsPipelineStateHash);
		}
	}

	void MaterialTechnique::scheduleForShaderUniformUpdate()
	{
		MaterialBufferManager* materialBufferManager = getMaterialBufferManager();
		if (nullptr != materialBufferManager)
		{
			materialBufferManager->scheduleForUpdate(*this);
		}
	}

	void MaterialTechnique::fillCommandBuffer(const IRendererRuntime& rendererRuntime, uint32_t& textureResourceGroupRootParameterIndex, Renderer::IResourceGroup** textureResourceGroup)
	{
		// Set textures
		const Textures& textures = getTextures(rendererRuntime);
		if (textures.empty())
		{
			setInvalid(textureResourceGroupRootParameterIndex);
			*textureResourceGroup = nullptr;
		}
		else
		{
			// Create texture resource group, if needed
			if (nullptr == mTextureResourceGroup)
			{
				// Check texture resources
				const size_t numberOfTextures = textures.size();
				const TextureResourceManager& textureResourceManager = rendererRuntime.getTextureResourceManager();
				for (size_t i = 0; i < numberOfTextures; ++i)
				{
					// Due to background texture loading, some textures might not be ready, yet
					// -> But even in this situation there should be a decent fallback texture in place
					const Texture& texture = textures[i];
					TextureResource* textureResource = textureResourceManager.tryGetById(texture.textureResourceId);
					if (nullptr == textureResource)
					{
						// Maybe it's a dynamically created texture like a shadow map created by "RendererRuntime::CompositorInstancePassShadowMap"
						// which might not have been ready yet when the material was originally loaded
						textureResource = textureResourceManager.getTextureResourceByAssetId(texture.materialProperty.getTextureAssetIdValue());
						if (nullptr != textureResource)
						{
							mTextures[i].textureResourceId = textureResource->getId();
						}
					}
					if (nullptr != textureResource)
					{
						// We also need to get informed in case e.g. dynamic compositor textures get changed in order to update the texture resource group accordantly
						textureResource->connectResourceListener(*this);
					}
				}

				// Get material blueprint resource
				const MaterialBlueprintResource* materialBlueprintResource = getMaterialResourceManager().getRendererRuntime().getMaterialBlueprintResourceManager().tryGetById(mMaterialBlueprintResourceId);
				assert(nullptr != materialBlueprintResource);

				// Create texture resource group
				std::vector<Renderer::IResource*> textureResources;
				std::vector<Renderer::ISamplerState*> samplerStates;
				textureResources.resize(numberOfTextures);
				samplerStates.resize(numberOfTextures);
				const MaterialBlueprintResource::Textures& materialBlueprintResourceTextures = materialBlueprintResource->getTextures();
				const MaterialBlueprintResource::SamplerStates& materialBlueprintResourceSamplerStates = materialBlueprintResource->getSamplerStates();
				for (size_t i = 0; i < numberOfTextures; ++i)
				{
					// Set texture resource
					TextureResource* textureResource = textureResourceManager.tryGetById(textures[i].textureResourceId);
					assert(nullptr != textureResource);
					textureResources[i] = textureResource->getTexture();
					assert(nullptr != textureResources[i]);

					// Set sampler state, if there's one (e.g. texel fetch instead of sampling might be used)
					if (isValid(materialBlueprintResourceTextures[i].samplerStateIndex))
					{
						assert(materialBlueprintResourceTextures[i].samplerStateIndex < materialBlueprintResourceSamplerStates.size());
						samplerStates[i] = materialBlueprintResourceSamplerStates[materialBlueprintResourceTextures[i].samplerStateIndex].samplerStatePtr;
					}
					else
					{
						samplerStates[i] = nullptr;
					}
				}
				// TODO(co) All textures need to be inside the same resource group, this needs to be guaranteed by design
				mTextureResourceGroup = rendererRuntime.getRendererResourceManager().createResourceGroup(*materialBlueprintResource->getRootSignaturePtr(), textures[0].rootParameterIndex, static_cast<uint32_t>(numberOfTextures), textureResources.data(), samplerStates.data());
				RENDERER_SET_RESOURCE_DEBUG_NAME(mTextureResourceGroup, "Material technique")
			}

			// Tell the caller about the resource group
			textureResourceGroupRootParameterIndex = textures[0].rootParameterIndex;
			*textureResourceGroup = mTextureResourceGroup;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
