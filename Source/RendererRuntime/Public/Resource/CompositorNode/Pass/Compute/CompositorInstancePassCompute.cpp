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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Compute/CompositorInstancePassCompute.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Core/IProfiler.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CompositorInstancePassCompute::CompositorInstancePassCompute(const CompositorResourcePassCompute& compositorResourcePassCompute, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassCompute, compositorNodeInstance),
		mComputeMaterialBlueprint(true),
		mRenderQueue(compositorNodeInstance.getCompositorWorkspaceInstance().getRendererRuntime().getMaterialBlueprintResourceManager().getIndirectBufferManager(), 0, 0, false, false),
		mMaterialResourceId(getInvalid<MaterialResourceId>())
	{
		// Sanity checks
		assert(!compositorResourcePassCompute.isMaterialDefinitionMandatory() || isValid(compositorResourcePassCompute.getMaterialAssetId()) || isValid(compositorResourcePassCompute.getMaterialBlueprintAssetId()));
		assert(!(isValid(compositorResourcePassCompute.getMaterialAssetId()) && isValid(compositorResourcePassCompute.getMaterialBlueprintAssetId())));

		// Get parent material resource ID and initiate creating the compositor instance pass compute material resource
		MaterialResourceManager& materialResourceManager = compositorNodeInstance.getCompositorWorkspaceInstance().getRendererRuntime().getMaterialResourceManager();
		if (isValid(compositorResourcePassCompute.getMaterialAssetId()))
		{
			// Get or load material resource
			MaterialResourceId materialResourceId = getInvalid<MaterialResourceId>();
			materialResourceManager.loadMaterialResourceByAssetId(compositorResourcePassCompute.getMaterialAssetId(), materialResourceId, this);
		}
		else
		{
			// Get or load material blueprint resource
			const AssetId materialBlueprintAssetId = compositorResourcePassCompute.getMaterialBlueprintAssetId();
			if (isValid(materialBlueprintAssetId))
			{
				MaterialResourceId parentMaterialResourceId = materialResourceManager.getMaterialResourceIdByAssetId(materialBlueprintAssetId);
				if (isInvalid(parentMaterialResourceId))
				{
					parentMaterialResourceId = materialResourceManager.createMaterialResourceByAssetId(materialBlueprintAssetId, materialBlueprintAssetId, compositorResourcePassCompute.getMaterialTechniqueId());
				}
				createMaterialResource(parentMaterialResourceId);
			}
		}
	}

	CompositorInstancePassCompute::~CompositorInstancePassCompute()
	{
		if (isValid(mMaterialResourceId))
		{
			// Clear the renderable manager
			mRenderableManager.getRenderables().clear();

			// Destroy the material resource the compositor instance pass compute created
			getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getMaterialResourceManager().destroyMaterialResource(mMaterialResourceId);
		}
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassCompute::onFillCommandBuffer(const Renderer::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		if (isValid(mMaterialResourceId))
		{
			// Sanity check
			assert(!mRenderableManager.getRenderables().empty());

			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), commandBuffer, getCompositorResourcePass().getDebugName())

			// Fill command buffer depending on graphics or compute material blueprint
			mRenderQueue.addRenderablesFromRenderableManager(mRenderableManager);
			if (mRenderQueue.getNumberOfDrawCalls() > 0)
			{
				if (mComputeMaterialBlueprint)
				{
					// Sanity check
					assert((nullptr == renderTarget) && "The compute compositor instance pass needs an invalid render target in case a compute material blueprint is used");

					// Fill command buffer using a compute material blueprint
					mRenderQueue.fillComputeCommandBuffer(static_cast<const CompositorResourcePassCompute&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData, commandBuffer);
				}
				else
				{
					// Sanity check
					assert((nullptr != renderTarget) && "The compute compositor instance pass needs a valid render target in case a graphics material blueprint is used");

					// Fill command buffer using a graphics material blueprint
					mRenderQueue.fillGraphicsCommandBuffer(*renderTarget, static_cast<const CompositorResourcePassCompute&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData, commandBuffer);
				}
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassCompute::onLoadingStateChange(const IResource& resource)
	{
		assert(resource.getId() == mMaterialResourceId);
		createMaterialResource(resource.getId());
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::CompositorInstancePassCompute methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassCompute::createMaterialResource(MaterialResourceId parentMaterialResourceId)
	{
		// Sanity checks
		assert(isInvalid(mMaterialResourceId));
		assert(isValid(parentMaterialResourceId));

		// Each compositor instance pass compute must have its own material resource since material property values might vary
		const IRendererRuntime& rendererRuntime = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime();
		MaterialResourceManager& materialResourceManager = rendererRuntime.getMaterialResourceManager();
		mMaterialResourceId = materialResourceManager.createMaterialResourceByCloning(parentMaterialResourceId);

		// Graphics or compute material blueprint?
		mComputeMaterialBlueprint = true;
		MaterialResource& materialResource = materialResourceManager.getById(mMaterialResourceId);
		{
			const MaterialTechnique* materialTechnique = materialResource.getMaterialTechniqueById(MaterialResourceManager::DEFAULT_MATERIAL_TECHNIQUE_ID);
			assert(nullptr != materialTechnique);
			MaterialBlueprintResource* materialBlueprintResource = rendererRuntime.getMaterialBlueprintResourceManager().tryGetById(materialTechnique->getMaterialBlueprintResourceId());
			assert(nullptr != materialBlueprintResource);
			mComputeMaterialBlueprint = isValid(materialBlueprintResource->getComputeShaderBlueprintResourceId());
		}

		{ // Set compositor resource pass compute material properties
			const MaterialProperties::SortedPropertyVector& sortedPropertyVector = static_cast<const CompositorResourcePassCompute&>(getCompositorResourcePass()).getMaterialProperties().getSortedPropertyVector();
			if (!sortedPropertyVector.empty())
			{
				for (const MaterialProperty& materialProperty : sortedPropertyVector)
				{
					if (materialProperty.isOverwritten())
					{
						materialResource.setPropertyById(materialProperty.getMaterialPropertyId(), materialProperty);
					}
				}
			}
		}

		// Setup renderable manager using attribute-less rendering
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, Renderer::IVertexArrayPtr(), materialResourceManager, mMaterialResourceId, getInvalid<SkeletonResourceId>(), false, 0, 3);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
