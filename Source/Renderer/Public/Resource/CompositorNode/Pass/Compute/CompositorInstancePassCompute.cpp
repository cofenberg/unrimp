/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Renderer/Public/Resource/CompositorNode/Pass/Compute/CompositorInstancePassCompute.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialResource.h"
#include "Renderer/Public/Core/IProfiler.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CompositorInstancePassCompute::CompositorInstancePassCompute(const CompositorResourcePassCompute& compositorResourcePassCompute, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassCompute, compositorNodeInstance),
		mComputeMaterialBlueprint(true),
		mRenderQueue(compositorNodeInstance.getCompositorWorkspaceInstance().getRenderer().getMaterialBlueprintResourceManager().getIndirectBufferManager(), 0, 0, false, false, false),
		mMaterialResourceId(getInvalid<MaterialResourceId>())
	{
		const IRenderer& renderer = compositorNodeInstance.getCompositorWorkspaceInstance().getRenderer();

		// Sanity checks
		RHI_ASSERT(renderer.getContext(), !compositorResourcePassCompute.isMaterialDefinitionMandatory() || isValid(compositorResourcePassCompute.getMaterialAssetId()) || isValid(compositorResourcePassCompute.getMaterialBlueprintAssetId()), "Invalid compositor resource pass compute configuration")
		RHI_ASSERT(renderer.getContext(), !(isValid(compositorResourcePassCompute.getMaterialAssetId()) && isValid(compositorResourcePassCompute.getMaterialBlueprintAssetId())), "Invalid compositor resource pass compute configuration")

		// Get parent material resource ID and initiate creating the compositor instance pass compute material resource
		MaterialResourceManager& materialResourceManager = renderer.getMaterialResourceManager();
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
			getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getMaterialResourceManager().destroyMaterialResource(mMaterialResourceId);
		}
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassCompute::onFillCommandBuffer(const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer)
	{
		if (isValid(mMaterialResourceId))
		{
			// Sanity check
			RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), !mRenderableManager.getRenderables().empty(), "No renderables")

			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), commandBuffer, getCompositorResourcePass().getDebugName())

			// Fill command buffer depending on graphics or compute material blueprint
			mRenderQueue.addRenderablesFromRenderableManager(mRenderableManager, static_cast<const CompositorResourcePassCompute&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData);
			if (mRenderQueue.getNumberOfDrawCalls() > 0)
			{
				if (mComputeMaterialBlueprint)
				{
					// Sanity check
					RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr == renderTarget, "The compute compositor instance pass needs an invalid render target in case a compute material blueprint is used")

					// Fill command buffer using a compute material blueprint
					mRenderQueue.fillComputeCommandBuffer(compositorContextData, commandBuffer);
				}
				else
				{
					// Sanity check
					RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != renderTarget, "The compute compositor instance pass needs a valid render target in case a graphics material blueprint is used")

					// Fill command buffer using a graphics material blueprint
					mRenderQueue.fillGraphicsCommandBuffer(*renderTarget, compositorContextData, commandBuffer);
				}
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::IResourceListener methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassCompute::onLoadingStateChange(const IResource& resource)
	{
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), resource.getId() == mMaterialResourceId, "Invalid material resource ID")
		createMaterialResource(resource.getId());
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::CompositorInstancePassCompute methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassCompute::createMaterialResource(MaterialResourceId parentMaterialResourceId)
	{
		const IRenderer& renderer = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer();

		// Sanity checks
		RHI_ASSERT(renderer.getContext(), isInvalid(mMaterialResourceId), "Invalid material resource ID")
		RHI_ASSERT(renderer.getContext(), isValid(parentMaterialResourceId), "Invalid material resource ID")

		// Each compositor instance pass compute must have its own material resource since material property values might vary
		MaterialResourceManager& materialResourceManager = renderer.getMaterialResourceManager();
		mMaterialResourceId = materialResourceManager.createMaterialResourceByCloning(parentMaterialResourceId);

		// Graphics or compute material blueprint?
		mComputeMaterialBlueprint = true;
		MaterialResource& materialResource = materialResourceManager.getById(mMaterialResourceId);
		{
			const MaterialTechnique* materialTechnique = materialResource.getMaterialTechniqueById(MaterialResourceManager::DEFAULT_MATERIAL_TECHNIQUE_ID);
			RHI_ASSERT(renderer.getContext(), nullptr != materialTechnique, "Invalid material technique")
			MaterialBlueprintResource* materialBlueprintResource = renderer.getMaterialBlueprintResourceManager().tryGetById(materialTechnique->getMaterialBlueprintResourceId());
			RHI_ASSERT(renderer.getContext(), nullptr != materialBlueprintResource, "Invalid material blueprint resource")
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
		#ifdef RHI_DEBUG
			mRenderableManager.setDebugName(materialResource.getDebugName());
		#endif
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, Rhi::IVertexArrayPtr(), materialResourceManager, mMaterialResourceId, getInvalid<SkeletonResourceId>(), false, 0, 3, 1 RHI_RESOURCE_DEBUG_NAME(materialResource.getDebugName()));
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
