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
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Loader/MaterialBlueprintResourceLoader.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Loader/MaterialBlueprintFileFormat.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/PassBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/VertexAttributes/VertexAttributesResourceManager.h"
#include "RendererRuntime/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	void MaterialBlueprintResourceLoader::initialize(const Asset& asset, bool reload, IResource& resource)
	{
		IResourceLoader::initialize(asset, reload);
		mMaterialBlueprintResource = static_cast<MaterialBlueprintResource*>(&resource);
	}

	bool MaterialBlueprintResourceLoader::onDeserialization(IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data
		return mMemoryFile.loadLz4CompressedDataFromFile(v1MaterialBlueprint::FORMAT_TYPE, v1MaterialBlueprint::FORMAT_VERSION, file);
	}

	void MaterialBlueprintResourceLoader::onProcessing()
	{
		// Decompress LZ4 compressed data
		mMemoryFile.decompress();

		// Read in the material blueprint header
		v1MaterialBlueprint::MaterialBlueprintHeader materialBlueprintHeader;
		mMemoryFile.read(&materialBlueprintHeader, sizeof(v1MaterialBlueprint::MaterialBlueprintHeader));

		{ // Read properties
			// TODO(co) Get rid of the evil const-cast
			MaterialProperties::SortedPropertyVector& sortedPropertyVector = const_cast<MaterialProperties::SortedPropertyVector&>(mMaterialBlueprintResource->mMaterialProperties.getSortedPropertyVector());
			if (materialBlueprintHeader.numberOfProperties > 0)
			{
				sortedPropertyVector.resize(materialBlueprintHeader.numberOfProperties);
				mMemoryFile.read(sortedPropertyVector.data(), sizeof(MaterialProperty) * materialBlueprintHeader.numberOfProperties);
			}
			else
			{
				sortedPropertyVector.clear();
			}
		}

		{ // Read visual importance of shader properties
			// TODO(co) Get rid of the evil const-cast
			ShaderProperties::SortedPropertyVector& sortedPropertyVector = const_cast<ShaderProperties::SortedPropertyVector&>(mMaterialBlueprintResource->mVisualImportanceOfShaderProperties.getSortedPropertyVector());
			if (materialBlueprintHeader.numberOfShaderCombinationProperties > 0)
			{
				sortedPropertyVector.resize(materialBlueprintHeader.numberOfShaderCombinationProperties);
				mMemoryFile.read(sortedPropertyVector.data(), sizeof(ShaderProperties::Property) * materialBlueprintHeader.numberOfShaderCombinationProperties);
			}
			else
			{
				sortedPropertyVector.clear();
			}
		}

		{ // Read maximum integer value of shader properties
			// TODO(co) Get rid of the evil const-cast
			ShaderProperties::SortedPropertyVector& sortedPropertyVector = const_cast<ShaderProperties::SortedPropertyVector&>(mMaterialBlueprintResource->mMaximumIntegerValueOfShaderProperties.getSortedPropertyVector());
			if (materialBlueprintHeader.numberOfIntegerShaderCombinationProperties > 0)
			{
				sortedPropertyVector.resize(materialBlueprintHeader.numberOfIntegerShaderCombinationProperties);
				mMemoryFile.read(sortedPropertyVector.data(), sizeof(ShaderProperties::Property) * materialBlueprintHeader.numberOfIntegerShaderCombinationProperties);
			}
			else
			{
				sortedPropertyVector.clear();
			}
		}

		{ // Read in the root signature
			// Read in the root signature header
			v1MaterialBlueprint::RootSignatureHeader rootSignatureHeader;
			mMemoryFile.read(&rootSignatureHeader, sizeof(v1MaterialBlueprint::RootSignatureHeader));
			RENDERER_ASSERT(mRendererRuntime.getContext(), rootSignatureHeader.numberOfRootParameters > 0 || 0 == rootSignatureHeader.numberOfDescriptorRanges, "Invalid root signature without root parameters but with descriptor ranges detected")

			// Load in root signature data
			if (rootSignatureHeader.numberOfRootParameters > 0)
			{
				// Allocate memory for the temporary data
				if (mMaximumNumberOfRootParameters < rootSignatureHeader.numberOfRootParameters)
				{
					mMaximumNumberOfRootParameters = rootSignatureHeader.numberOfRootParameters;
					mRootParameters.resize(mMaximumNumberOfRootParameters);
				}
				if (mMaximumNumberOfDescriptorRanges < rootSignatureHeader.numberOfDescriptorRanges)
				{
					mMaximumNumberOfDescriptorRanges = rootSignatureHeader.numberOfDescriptorRanges;
					mDescriptorRanges.resize(mMaximumNumberOfDescriptorRanges);
				}

				// Load in the root parameters
				std::vector<Renderer::RootParameterData> rootParameterData;
				rootParameterData.resize(rootSignatureHeader.numberOfRootParameters);
				mMemoryFile.read(rootParameterData.data(), sizeof(Renderer::RootParameterData) * rootSignatureHeader.numberOfRootParameters);
				for (size_t index = 0; index < rootSignatureHeader.numberOfRootParameters; ++index)
				{
					mRootParameters[index].parameterType = rootParameterData[index].parameterType;
					mRootParameters[index].descriptorTable.numberOfDescriptorRanges = rootParameterData[index].numberOfDescriptorRanges;
				}

				// Load in the descriptor ranges
				if (rootSignatureHeader.numberOfDescriptorRanges > 0)
				{
					mMemoryFile.read(mDescriptorRanges.data(), sizeof(Renderer::DescriptorRange) * rootSignatureHeader.numberOfDescriptorRanges);
				}
				else
				{
					mDescriptorRanges.clear();
				}
			}
			else
			{
				mRootParameters.clear();
				mDescriptorRanges.clear();
			}

			// Prepare our temporary root signature
			mRootSignature.numberOfParameters	  = rootSignatureHeader.numberOfRootParameters;
			mRootSignature.parameters			  = mRootParameters.data();
			mRootSignature.numberOfStaticSamplers = rootSignatureHeader.numberOfStaticSamplers;
			mRootSignature.staticSamplers		  = nullptr;	// TODO(co) Add support for static samplers
			mRootSignature.flags				  = static_cast<Renderer::RootSignatureFlags::Enum>(rootSignatureHeader.flags);

			{ // Tell the temporary root signature about the descriptor ranges
				Renderer::DescriptorRange* descriptorRange = mDescriptorRanges.data();
				for (uint32_t i = 0; i < rootSignatureHeader.numberOfRootParameters; ++i)
				{
					Renderer::RootParameter& rootParameter = mRootParameters[i];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						rootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(descriptorRange);
						descriptorRange += rootParameter.descriptorTable.numberOfDescriptorRanges;
					}
				}
			}
		}

		// A material blueprint can have a compute or a graphics pipeline state, but never both at one and the same time
		// -> Read in the compute pipeline state: Read in the compute shader blueprint
		mMemoryFile.read(&mComputeShaderBlueprintAssetId, sizeof(AssetId));

		// Read in the graphics pipeline state in case there's no compute pipeline state
		if (isInvalid(mComputeShaderBlueprintAssetId))
		{
			// Read vertex attributes asset ID
			mMemoryFile.read(&mVertexAttributesAssetId, sizeof(AssetId));

			// Read in the shader blueprints
			mMemoryFile.read(&mGraphicsShaderBlueprintAssetId, sizeof(AssetId) * NUMBER_OF_GRAPHICS_SHADER_TYPES);

			// Read in the graphics pipeline state
			mMemoryFile.read(&mMaterialBlueprintResource->mGraphicsPipelineState, sizeof(Renderer::SerializedGraphicsPipelineState));
			mMaterialBlueprintResource->mGraphicsPipelineState.rootSignature = nullptr;
			mMaterialBlueprintResource->mGraphicsPipelineState.graphicsProgram = nullptr;
			mMaterialBlueprintResource->mGraphicsPipelineState.vertexAttributes.numberOfAttributes = 0;
			mMaterialBlueprintResource->mGraphicsPipelineState.vertexAttributes.attributes = nullptr;
		}

		{ // Read in the uniform buffers
			MaterialBlueprintResource::UniformBuffers& uniformBuffers = mMaterialBlueprintResource->mUniformBuffers;
			uniformBuffers.resize(materialBlueprintHeader.numberOfUniformBuffers);
			for (uint32_t i = 0; i < materialBlueprintHeader.numberOfUniformBuffers; ++i)
			{
				MaterialBlueprintResource::UniformBuffer& uniformBuffer = uniformBuffers[i];

				// Read in the uniform buffer header
				v1MaterialBlueprint::UniformBufferHeader uniformBufferHeader;
				mMemoryFile.read(&uniformBufferHeader, sizeof(v1MaterialBlueprint::UniformBufferHeader));
				uniformBuffer.rootParameterIndex		 = uniformBufferHeader.rootParameterIndex;
				uniformBuffer.bufferUsage				 = uniformBufferHeader.bufferUsage;
				uniformBuffer.numberOfElements			 = uniformBufferHeader.numberOfElements;
				uniformBuffer.uniformBufferNumberOfBytes = uniformBufferHeader.uniformBufferNumberOfBytes;

				// Sanity check
				RENDERER_ASSERT(mRendererRuntime.getContext(), uniformBufferHeader.numberOfElementProperties > 0, "Invalid uniform buffer without any element properties detected")

				// Read in the uniform buffer property elements
				MaterialBlueprintResource::UniformBufferElementProperties& uniformBufferElementProperties = uniformBuffer.uniformBufferElementProperties;
				uniformBufferElementProperties.resize(uniformBufferHeader.numberOfElementProperties);
				mMemoryFile.read(uniformBufferElementProperties.data(), sizeof(MaterialProperty) * uniformBufferHeader.numberOfElementProperties);
			}
		}

		{ // Read in the texture buffers
			MaterialBlueprintResource::TextureBuffers& textureBuffers = mMaterialBlueprintResource->mTextureBuffers;
			textureBuffers.resize(materialBlueprintHeader.numberOfTextureBuffers);
			for (uint32_t i = 0; i < materialBlueprintHeader.numberOfTextureBuffers; ++i)
			{
				MaterialBlueprintResource::TextureBuffer& textureBuffer = textureBuffers[i];

				// Read in the texture buffer header
				v1MaterialBlueprint::TextureBufferHeader textureBufferHeader;
				mMemoryFile.read(&textureBufferHeader, sizeof(v1MaterialBlueprint::TextureBufferHeader));
				textureBuffer.materialPropertyValue = textureBufferHeader.materialPropertyValue;
				textureBuffer.rootParameterIndex	= textureBufferHeader.rootParameterIndex;
				textureBuffer.bufferUsage			= textureBufferHeader.bufferUsage;
			}
		}

		// Read in the sampler states
		if (materialBlueprintHeader.numberOfSamplerStates > 0)
		{
			// Allocate memory for the temporary data
			if (mMaximumNumberOfMaterialBlueprintSamplerStates < materialBlueprintHeader.numberOfSamplerStates)
			{
				delete [] mMaterialBlueprintSamplerStates;
				mMaximumNumberOfMaterialBlueprintSamplerStates = materialBlueprintHeader.numberOfSamplerStates;
				mMaterialBlueprintSamplerStates = new v1MaterialBlueprint::SamplerState[mMaximumNumberOfMaterialBlueprintSamplerStates];
			}

			// Read in the sampler states
			mMemoryFile.read(mMaterialBlueprintSamplerStates, sizeof(v1MaterialBlueprint::SamplerState) * materialBlueprintHeader.numberOfSamplerStates);

			// Allocate material blueprint resource sampler states
			mMaterialBlueprintResource->mSamplerStates.resize(materialBlueprintHeader.numberOfSamplerStates);
		}
		else
		{
			mMaterialBlueprintResource->mSamplerStates.clear();
		}

		// Read in the textures
		if (materialBlueprintHeader.numberOfTextures > 0)
		{
			// Allocate memory for the temporary data
			if (mMaximumNumberOfMaterialBlueprintTextures < materialBlueprintHeader.numberOfTextures)
			{
				delete [] mMaterialBlueprintTextures;
				mMaximumNumberOfMaterialBlueprintTextures = materialBlueprintHeader.numberOfTextures;
				mMaterialBlueprintTextures = new v1MaterialBlueprint::Texture[mMaximumNumberOfMaterialBlueprintTextures];
			}

			// Read in the textures
			mMemoryFile.read(mMaterialBlueprintTextures, sizeof(v1MaterialBlueprint::Texture) * materialBlueprintHeader.numberOfTextures);

			// Allocate material blueprint resource textures
			mMaterialBlueprintResource->mTextures.resize(materialBlueprintHeader.numberOfTextures);
		}
		else
		{
			mMaterialBlueprintResource->mTextures.clear();
		}

		// Can we create the renderer resources asynchronous as well?
		if (mRendererRuntime.getRenderer().getCapabilities().nativeMultiThreading)
		{
			createRendererResources();
		}
	}

	bool MaterialBlueprintResourceLoader::onDispatch()
	{
		// Create the renderer resources, in case it wasn't already done asynchronously
		if (!mRendererRuntime.getRenderer().getCapabilities().nativeMultiThreading)
		{
			createRendererResources();
		}

		{ // Graphics pipeline state
			// Get the used vertex attributes resource
			mRendererRuntime.getVertexAttributesResourceManager().loadVertexAttributesResourceByAssetId(mVertexAttributesAssetId, mMaterialBlueprintResource->mVertexAttributesResourceId);

			{ // Get the used shader blueprint resources
				ShaderBlueprintResourceManager& shaderBlueprintResourceManager = mRendererRuntime.getShaderBlueprintResourceManager();
				if (isValid(mComputeShaderBlueprintAssetId))
				{
					// The material blueprint is using a compute pipeline state
					shaderBlueprintResourceManager.loadShaderBlueprintResourceByAssetId(mComputeShaderBlueprintAssetId, mMaterialBlueprintResource->mComputeShaderBlueprintResourceId);
				}
				else
				{
					// The material blueprint is using a graphics pipeline state
					for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
					{
						if (isValid(mGraphicsShaderBlueprintAssetId[i]))
						{
							shaderBlueprintResourceManager.loadShaderBlueprintResourceByAssetId(mGraphicsShaderBlueprintAssetId[i], mMaterialBlueprintResource->mGraphicsShaderBlueprintResourceId[i]);
						}
					}
				}
			}
		}

		{ // Gather ease-of-use direct access to resources
			MaterialBlueprintResource::UniformBuffers& uniformBuffers = mMaterialBlueprintResource->mUniformBuffers;
			const size_t numberOfUniformBuffers = uniformBuffers.size();
			for (size_t i = 0; i < numberOfUniformBuffers; ++i)
			{
				MaterialBlueprintResource::UniformBuffer& uniformBuffer = uniformBuffers[i];
				switch (uniformBuffer.bufferUsage)
				{
					case MaterialBlueprintResource::BufferUsage::UNKNOWN:
						// Nothing here
						break;

					case MaterialBlueprintResource::BufferUsage::PASS:
						mMaterialBlueprintResource->mPassUniformBuffer = &uniformBuffer;
						break;

					case MaterialBlueprintResource::BufferUsage::MATERIAL:
						mMaterialBlueprintResource->mMaterialUniformBuffer = &uniformBuffer;
						break;

					case MaterialBlueprintResource::BufferUsage::INSTANCE:
						mMaterialBlueprintResource->mInstanceUniformBuffer = &uniformBuffer;
						break;

					case MaterialBlueprintResource::BufferUsage::LIGHT:
						// Error!
						RENDERER_ASSERT(mRendererRuntime.getContext(), false, "Invalid buffer usage")
						break;
				}
			}
		}

		{ // Gather ease-of-use direct access to resources
			MaterialBlueprintResource::TextureBuffers& textureBuffers = mMaterialBlueprintResource->mTextureBuffers;
			const size_t numberOfTextureBuffers = textureBuffers.size();
			for (size_t i = 0; i < numberOfTextureBuffers; ++i)
			{
				MaterialBlueprintResource::TextureBuffer& textureBuffer = textureBuffers[i];
				switch (textureBuffer.bufferUsage)
				{
					case MaterialBlueprintResource::BufferUsage::UNKNOWN:
					case MaterialBlueprintResource::BufferUsage::PASS:
					case MaterialBlueprintResource::BufferUsage::MATERIAL:
						// Nothing here
						break;

					case MaterialBlueprintResource::BufferUsage::INSTANCE:
						mMaterialBlueprintResource->mInstanceTextureBuffer = &textureBuffer;
						break;

					case MaterialBlueprintResource::BufferUsage::LIGHT:
						mMaterialBlueprintResource->mLightTextureBuffer = &textureBuffer;
						break;
				}
			}
		}

		// Create pass buffer manager
		delete mMaterialBlueprintResource->mPassBufferManager;
		mMaterialBlueprintResource->mPassBufferManager = new PassBufferManager(mRendererRuntime, *mMaterialBlueprintResource);

		// Create material buffer manager
		if (nullptr != mMaterialBlueprintResource->mMaterialBufferManager)
		{
			delete mMaterialBlueprintResource->mMaterialBufferManager;
			mMaterialBlueprintResource->mMaterialBufferManager = nullptr;
		}
		{ // It's valid if a material blueprint resource doesn't contain a material uniform buffer (usually the case for compositor material blueprint resources)
			const MaterialBlueprintResource::UniformBuffer* uniformBuffer = mMaterialBlueprintResource->getMaterialUniformBuffer();
			if (nullptr != uniformBuffer && mRendererRuntime.getRenderer().getCapabilities().maximumUniformBufferSize > 0)
			{
				mMaterialBlueprintResource->mMaterialBufferManager = new MaterialBufferManager(mRendererRuntime, *mMaterialBlueprintResource);
			}
		}

		{ // Get the textures
			TextureResourceManager& textureResourceManager = mRendererRuntime.getTextureResourceManager();
			MaterialBlueprintResource::Textures& textures = mMaterialBlueprintResource->mTextures;
			const size_t numberOfTextures = textures.size();
			const v1MaterialBlueprint::Texture* materialBlueprintTexture = mMaterialBlueprintTextures;
			for (size_t i = 0; i < numberOfTextures; ++i, ++materialBlueprintTexture)
			{
				MaterialBlueprintResource::Texture& texture = textures[i];
				texture.rootParameterIndex = materialBlueprintTexture->rootParameterIndex;
				const MaterialProperty& materialProperty = texture.materialProperty = materialBlueprintTexture->materialProperty;
				texture.fallbackTextureAssetId = materialBlueprintTexture->fallbackTextureAssetId;
				texture.rgbHardwareGammaCorrection = materialBlueprintTexture->rgbHardwareGammaCorrection;
				texture.samplerStateIndex = materialBlueprintTexture->samplerStateIndex;
				if (materialProperty.getValueType() == MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID)
				{
					textureResourceManager.loadTextureResourceByAssetId(materialProperty.getTextureAssetIdValue(), texture.fallbackTextureAssetId, texture.textureResourceId, nullptr, texture.rgbHardwareGammaCorrection);
				}
			}
		}

		{ // Register the global material properties
			#define GET_MATERIAL_PROPERTY_USAGE \
				MaterialProperty::Usage materialPropertyUsage = MaterialProperty::Usage::SHADER_UNIFORM; \
				for (const MaterialProperty& usageMaterialProperty : sortedPropertyVector) \
				{ \
					if (usageMaterialProperty.getValueType() == MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID && usageMaterialProperty.getGlobalMaterialPropertyId() == materialPropertyId) \
					{ \
						materialPropertyUsage = usageMaterialProperty.getUsage(); \
						break; \
					} \
				}
			MaterialProperties& globalMaterialProperties = mMaterialBlueprintResource->getResourceManager<MaterialBlueprintResourceManager>().getGlobalMaterialProperties();
			const MaterialProperties::SortedPropertyVector& sortedPropertyVector = mMaterialBlueprintResource->mMaterialProperties.getSortedPropertyVector();
			for (const MaterialProperty& materialProperty : sortedPropertyVector)
			{
				if (materialProperty.getUsage() == MaterialProperty::Usage::GLOBAL_REFERENCE_FALLBACK)
				{
					const MaterialPropertyId materialPropertyId = materialProperty.getMaterialPropertyId();
					const MaterialProperty* globalMaterialProperty = globalMaterialProperties.getPropertyById(materialPropertyId);
					if (nullptr == globalMaterialProperty)
					{
						GET_MATERIAL_PROPERTY_USAGE
						globalMaterialProperties.setPropertyById(materialPropertyId, materialProperty, materialPropertyUsage);
					}
					else
					{
						// Since the application user of the global material properties shouldn't need to care about the material property usage, it can happen that a global material property has been set by the user without having a known material property usage
						if (globalMaterialProperty->getUsage() == MaterialProperty::Usage::UNKNOWN)
						{
							GET_MATERIAL_PROPERTY_USAGE
							const_cast<MaterialProperty*>(globalMaterialProperty)->mUsage = materialPropertyUsage;
						}

						// Sanity check
						#ifdef _DEBUG
							GET_MATERIAL_PROPERTY_USAGE
							RENDERER_ASSERT(mRendererRuntime.getContext(), globalMaterialProperty->getValueType() == materialProperty.getValueType() && globalMaterialProperty->getUsage() == materialPropertyUsage, "Invalid property")
						#endif
					}
				}
			}
			#undef GET_MATERIAL_PROPERTY_USAGE
		}

		// Fully loaded?
		return isFullyLoaded();
	}

	bool MaterialBlueprintResourceLoader::isFullyLoaded()
	{
		// Vertex attributes resource
		if (IResource::LoadingState::LOADED != mRendererRuntime.getVertexAttributesResourceManager().getResourceByResourceId(mMaterialBlueprintResource->mVertexAttributesResourceId).getLoadingState())
		{
			// Not fully loaded
			return false;
		}

		// We only demand that all referenced shader blueprint resources are loaded, not yet loaded texture resources can be handled during runtime
		const ShaderBlueprintResourceManager& shaderBlueprintResourceManager = mRendererRuntime.getShaderBlueprintResourceManager();
		if (isValid(mComputeShaderBlueprintAssetId))
		{
			// The material blueprint is using a compute pipeline state
			const ShaderBlueprintResourceId shaderBlueprintResourceId = mMaterialBlueprintResource->mComputeShaderBlueprintResourceId;
			if (isValid(shaderBlueprintResourceId) && IResource::LoadingState::LOADED != shaderBlueprintResourceManager.getResourceByResourceId(shaderBlueprintResourceId).getLoadingState())
			{
				// Not fully loaded
				return false;
			}
		}
		else
		{
			// The material blueprint is using a graphics pipeline state
			for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
			{
				const ShaderBlueprintResourceId shaderBlueprintResourceId = mMaterialBlueprintResource->mGraphicsShaderBlueprintResourceId[i];
				if (isValid(shaderBlueprintResourceId) && IResource::LoadingState::LOADED != shaderBlueprintResourceManager.getResourceByResourceId(shaderBlueprintResourceId).getLoadingState())
				{
					// Not fully loaded
					return false;
				}
			}
		}

		// Fully loaded
		return true;
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	MaterialBlueprintResourceLoader::~MaterialBlueprintResourceLoader()
	{
		// Free temporary data
		delete [] mMaterialBlueprintSamplerStates;
		delete [] mMaterialBlueprintTextures;
	}

	void MaterialBlueprintResourceLoader::createRendererResources()
	{
		Renderer::IRenderer& renderer = mRendererRuntime.getRenderer();

		// Create the root signature
		mMaterialBlueprintResource->mRootSignaturePtr = renderer.createRootSignature(mRootSignature);
		RENDERER_SET_RESOURCE_DEBUG_NAME(mMaterialBlueprintResource->mRootSignaturePtr, getAsset().virtualFilename);

		// Create the sampler states
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mMaterialBlueprintResource->getResourceManager<MaterialBlueprintResourceManager>();
		const Renderer::FilterMode defaultTextureFilterMode = materialBlueprintResourceManager.getDefaultTextureFilterMode();
		const uint8_t defaultMaximumTextureAnisotropy = materialBlueprintResourceManager.getDefaultMaximumTextureAnisotropy();
		MaterialBlueprintResource::SamplerStates& samplerStates = mMaterialBlueprintResource->mSamplerStates;
		const size_t numberOfSamplerStates = samplerStates.size();
		v1MaterialBlueprint::SamplerState* materialBlueprintSamplerState = mMaterialBlueprintSamplerStates;
		for (size_t i = 0; i < numberOfSamplerStates; ++i, ++materialBlueprintSamplerState)
		{
			MaterialBlueprintResource::SamplerState& samplerState = samplerStates[i];
			memcpy(&samplerState.rendererSamplerState, &materialBlueprintSamplerState->samplerState, sizeof(Renderer::SamplerState));
			samplerState.rootParameterIndex = materialBlueprintSamplerState->rootParameterIndex;
			if (Renderer::FilterMode::UNKNOWN == materialBlueprintSamplerState->samplerState.filter)
			{
				materialBlueprintSamplerState->samplerState.filter = defaultTextureFilterMode;
			}
			if (isInvalid(materialBlueprintSamplerState->samplerState.maxAnisotropy))
			{
				materialBlueprintSamplerState->samplerState.maxAnisotropy = defaultMaximumTextureAnisotropy;
			}
			samplerState.samplerStatePtr = renderer.createSamplerState(materialBlueprintSamplerState->samplerState);
			RENDERER_SET_RESOURCE_DEBUG_NAME(samplerState.samplerStatePtr, getAsset().virtualFilename)
		}
		mMaterialBlueprintResource->mSamplerStateGroup = nullptr;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
