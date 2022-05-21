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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateSignature.h"
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateSignature.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "Renderer/Public/Core/Math/Math.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	ComputePipelineStateSignature::ComputePipelineStateSignature(const ComputePipelineStateSignature& computePipelineStateSignature) :
		mMaterialBlueprintResourceId(computePipelineStateSignature.mMaterialBlueprintResourceId),
		mShaderProperties(computePipelineStateSignature.mShaderProperties),
		mComputePipelineStateSignatureId(computePipelineStateSignature.mComputePipelineStateSignatureId),
		mShaderCombinationId(computePipelineStateSignature.mShaderCombinationId)
	{
		// Nothing here
	}

	ComputePipelineStateSignature& ComputePipelineStateSignature::operator=(const ComputePipelineStateSignature& computePipelineStateSignature)
	{
		mMaterialBlueprintResourceId = computePipelineStateSignature.mMaterialBlueprintResourceId;
		mShaderProperties = computePipelineStateSignature.mShaderProperties;
		mComputePipelineStateSignatureId = computePipelineStateSignature.mComputePipelineStateSignatureId;
		mShaderCombinationId = computePipelineStateSignature.mShaderCombinationId;

		// Done
		return *this;
	}

	void ComputePipelineStateSignature::set(const MaterialBlueprintResource& materialBlueprintResource, const ShaderProperties& shaderProperties)
	{
		mMaterialBlueprintResourceId		= materialBlueprintResource.getId();
		mShaderProperties					= shaderProperties;
		mComputePipelineStateSignatureId	= Math::FNV1a_INITIAL_HASH_32;
		mShaderCombinationId				= getInvalid<ShaderCombinationId>();

		// Incorporate primitive hashes
		mComputePipelineStateSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mMaterialBlueprintResourceId), sizeof(uint32_t), mComputePipelineStateSignatureId);

		// Incorporate shader related hash
		const ShaderBlueprintResource* shaderBlueprintResource = materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRenderer().getShaderBlueprintResourceManager().tryGetById(materialBlueprintResource.getComputeShaderBlueprintResourceId());
		if (nullptr != shaderBlueprintResource)
		{
			const uint32_t hash = mShaderCombinationId = GraphicsPipelineStateSignature::generateShaderCombinationId(*shaderBlueprintResource, mShaderProperties);
			mComputePipelineStateSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&hash), sizeof(uint32_t), mComputePipelineStateSignatureId);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
