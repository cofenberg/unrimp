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
#include "RendererRuntime/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h"
#include "RendererRuntime/Resource/CompositorNode/Loader/CompositorNodeFileFormat.h"
#include "RendererRuntime/Resource/Material/MaterialResourceManager.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CompositorResourcePassCompute::CompositorResourcePassCompute(const CompositorTarget& compositorTarget, AssetId materialBlueprintAssetId, const MaterialProperties& materialProperties) :
		ICompositorResourcePass(compositorTarget),
		mMaterialDefinitionMandatory(true),
		mMaterialTechniqueId(MaterialResourceManager::DEFAULT_MATERIAL_TECHNIQUE_ID),
		mMaterialBlueprintAssetId(materialBlueprintAssetId),
		mMaterialProperties(materialProperties)
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorResourcePass methods ]
	//[-------------------------------------------------------]
	void CompositorResourcePassCompute::deserialize(MAYBE_UNUSED uint32_t numberOfBytes, const uint8_t* data)
	{
		// Sanity check
		assert(sizeof(v1CompositorNode::PassCompute) <= numberOfBytes);

		// Call the base implementation
		ICompositorResourcePass::deserialize(sizeof(v1CompositorNode::Pass), data);

		// Read data
		const v1CompositorNode::PassCompute* passCompute = reinterpret_cast<const v1CompositorNode::PassCompute*>(data);
		assert(sizeof(v1CompositorNode::PassCompute) + sizeof(MaterialProperty) * passCompute->numberOfMaterialProperties == numberOfBytes);
		mMaterialAssetId = passCompute->materialAssetId;
		mMaterialTechniqueId = passCompute->materialTechniqueId;
		mMaterialBlueprintAssetId = passCompute->materialBlueprintAssetId;

		{ // Read material properties
			// TODO(co) Get rid of the evil const-cast
			MaterialProperties::SortedPropertyVector& sortedPropertyVector = const_cast<MaterialProperties::SortedPropertyVector&>(mMaterialProperties.getSortedPropertyVector());
			sortedPropertyVector.resize(passCompute->numberOfMaterialProperties);
			memcpy(reinterpret_cast<char*>(sortedPropertyVector.data()), data + sizeof(v1CompositorNode::PassCompute), sizeof(MaterialProperty) * passCompute->numberOfMaterialProperties);
		}

		// Sanity checks
		assert(!mMaterialDefinitionMandatory || isValid(mMaterialAssetId) || isValid(mMaterialBlueprintAssetId));
		assert(!(isValid(mMaterialAssetId) && isValid(mMaterialBlueprintAssetId)));
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
