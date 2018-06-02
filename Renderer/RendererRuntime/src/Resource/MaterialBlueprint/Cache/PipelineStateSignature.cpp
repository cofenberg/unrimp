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
#include "RendererRuntime/Resource/MaterialBlueprint/Cache/PipelineStateSignature.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "RendererRuntime/Core/Math/Math.h"
#include "RendererRuntime/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		RendererRuntime::ShaderCombinationId generateShaderCombinationId(const RendererRuntime::ShaderBlueprintResource& shaderBlueprintResource, const RendererRuntime::ShaderProperties& shaderProperties)
		{
			RendererRuntime::ShaderCombinationId shaderCombinationId(RendererRuntime::Math::FNV1a_INITIAL_HASH_32);

			{ // Apply shader blueprint resource ID
				const RendererRuntime::ShaderBlueprintResourceId shaderBlueprintResourceId = shaderBlueprintResource.getId();
				shaderCombinationId = RendererRuntime::Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&shaderBlueprintResourceId), sizeof(uint32_t), shaderCombinationId);
			}

			// Apply shader properties
			const RendererRuntime::ShaderProperties& referencedShaderProperties = shaderBlueprintResource.getReferencedShaderProperties();
			for (const RendererRuntime::ShaderProperties::Property& property : shaderProperties.getSortedPropertyVector())
			{
				// Use the additional information to reduce the shader properties in order to generate fewer combinations
				const RendererRuntime::ShaderPropertyId shaderPropertyId = property.shaderPropertyId;
				if (referencedShaderProperties.hasPropertyValue(shaderPropertyId))
				{
					// No need to check for zero-value shader properties in here, already optimized out by "RendererRuntime::MaterialBlueprintResource::optimizeShaderProperties()"

					// Apply shader property ID
					shaderCombinationId = RendererRuntime::Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&shaderPropertyId), sizeof(uint32_t), shaderCombinationId);

					// Apply shader property value
					shaderCombinationId = RendererRuntime::Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&property.value), sizeof(int32_t), shaderCombinationId);
				}
			}

			// Done
			return shaderCombinationId;
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
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	PipelineStateSignature::PipelineStateSignature(const PipelineStateSignature& pipelineStateSignature) :
		mMaterialBlueprintResourceId(pipelineStateSignature.mMaterialBlueprintResourceId),
		mSerializedPipelineStateHash(pipelineStateSignature.mSerializedPipelineStateHash),
		mShaderProperties(pipelineStateSignature.mShaderProperties),
		mPipelineStateSignatureId(pipelineStateSignature.mPipelineStateSignatureId),
		mShaderCombinationId{getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>()}
	{
		for (uint8_t i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
		{
			mShaderCombinationId[i] = pipelineStateSignature.mShaderCombinationId[i];
		}
	}

	PipelineStateSignature& PipelineStateSignature::operator=(const PipelineStateSignature& pipelineStateSignature)
	{
		mMaterialBlueprintResourceId = pipelineStateSignature.mMaterialBlueprintResourceId;
		mSerializedPipelineStateHash = pipelineStateSignature.mSerializedPipelineStateHash;
		mShaderProperties = pipelineStateSignature.mShaderProperties;
		mPipelineStateSignatureId = pipelineStateSignature.mPipelineStateSignatureId;
		for (uint8_t i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
		{
			mShaderCombinationId[i] = pipelineStateSignature.mShaderCombinationId[i];
		}

		// Done
		return *this;
	}

	void PipelineStateSignature::set(const MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedPipelineStateHash, const ShaderProperties& shaderProperties)
	{
		mMaterialBlueprintResourceId = materialBlueprintResource.getId();
		mSerializedPipelineStateHash = serializedPipelineStateHash;
		mShaderProperties			 = shaderProperties;
		mPipelineStateSignatureId	 = Math::FNV1a_INITIAL_HASH_32;
		for (uint8_t i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
		{
			mShaderCombinationId[i] = getUninitialized<ShaderCombinationId>();
		}

		// Incorporate primitive hashes
		mPipelineStateSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mMaterialBlueprintResourceId), sizeof(uint32_t), mPipelineStateSignatureId);
		mPipelineStateSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mSerializedPipelineStateHash), sizeof(uint32_t), mPipelineStateSignatureId);

		// Incorporate shader related hashes
		const ShaderBlueprintResourceManager& shaderBlueprintResourceManager = materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getShaderBlueprintResourceManager();
		for (uint8_t i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
		{
			const ShaderBlueprintResource* shaderBlueprintResource = shaderBlueprintResourceManager.tryGetById(materialBlueprintResource.getShaderBlueprintResourceId(static_cast<ShaderType>(i)));
			if (nullptr != shaderBlueprintResource)
			{
				const uint32_t hash = mShaderCombinationId[i] = ::detail::generateShaderCombinationId(*shaderBlueprintResource, mShaderProperties);
				mPipelineStateSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&hash), sizeof(uint32_t), mPipelineStateSignatureId);
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
