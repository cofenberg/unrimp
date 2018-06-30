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
#include "RendererRuntime/Resource/CompositorNode/Pass/Quad/CompositorInstancePassQuad.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/Quad/CompositorResourcePassQuad.h"
#include "RendererRuntime/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Core/IProfiler.h"
#include "RendererRuntime/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CompositorInstancePassQuad::CompositorInstancePassQuad(const CompositorResourcePassQuad& compositorResourcePassQuad, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassQuad, compositorNodeInstance),
		mRenderQueue(compositorNodeInstance.getCompositorWorkspaceInstance().getRendererRuntime().getMaterialBlueprintResourceManager().getIndirectBufferManager(), 0, 0, false, false),
		mMaterialResourceId(getInvalid<MaterialResourceId>())
	{
		// Sanity checks
		assert(!compositorResourcePassQuad.isMaterialDefinitionMandatory() || isValid(compositorResourcePassQuad.getMaterialAssetId()) || isValid(compositorResourcePassQuad.getMaterialBlueprintAssetId()));
		assert(!(isValid(compositorResourcePassQuad.getMaterialAssetId()) && isValid(compositorResourcePassQuad.getMaterialBlueprintAssetId())));

		// Get parent material resource ID and initiate creating the compositor instance pass quad material resource
		MaterialResourceManager& materialResourceManager = compositorNodeInstance.getCompositorWorkspaceInstance().getRendererRuntime().getMaterialResourceManager();
		if (isValid(compositorResourcePassQuad.getMaterialAssetId()))
		{
			// Get or load material resource
			MaterialResourceId materialResourceId = getInvalid<MaterialResourceId>();
			materialResourceManager.loadMaterialResourceByAssetId(compositorResourcePassQuad.getMaterialAssetId(), materialResourceId, this);
		}
		else
		{
			// Get or load material blueprint resource
			const AssetId materialBlueprintAssetId = compositorResourcePassQuad.getMaterialBlueprintAssetId();
			if (isValid(materialBlueprintAssetId))
			{
				MaterialResourceId parentMaterialResourceId = materialResourceManager.getMaterialResourceIdByAssetId(materialBlueprintAssetId);
				if (isInvalid(parentMaterialResourceId))
				{
					parentMaterialResourceId = materialResourceManager.createMaterialResourceByAssetId(materialBlueprintAssetId, materialBlueprintAssetId, compositorResourcePassQuad.getMaterialTechniqueId());
				}
				createMaterialResource(parentMaterialResourceId);
			}
		}
	}

	CompositorInstancePassQuad::~CompositorInstancePassQuad()
	{
		if (isValid(mMaterialResourceId))
		{
			// Clear the renderable manager
			mRenderableManager.getRenderables().clear();

			// Destroy the material resource the compositor instance pass quad created
			getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getMaterialResourceManager().destroyMaterialResource(mMaterialResourceId);
		}
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassQuad::onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		if (!mRenderableManager.getRenderables().empty())
		{
			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			RENDERER_SCOPED_PROFILER_EVENT_FUNCTION(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), commandBuffer)

			// Fill command buffer
			mRenderQueue.addRenderablesFromRenderableManager(mRenderableManager);
			if (mRenderQueue.getNumberOfDrawCalls() > 0)
			{
				mRenderQueue.fillCommandBuffer(renderTarget, static_cast<const CompositorResourcePassQuad&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData, commandBuffer);
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassQuad::onLoadingStateChange(const IResource& resource)
	{
		assert(resource.getId() == mMaterialResourceId);
		createMaterialResource(resource.getId());
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::CompositorInstancePassQuad methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassQuad::createMaterialResource(MaterialResourceId parentMaterialResourceId)
	{
		// Sanity checks
		assert(isInvalid(mMaterialResourceId));
		assert(isValid(parentMaterialResourceId));

		// Each compositor instance pass quad must have its own material resource since material property values might vary
		const IRendererRuntime& rendererRuntime = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime();
		MaterialResourceManager& materialResourceManager = rendererRuntime.getMaterialResourceManager();
		mMaterialResourceId = materialResourceManager.createMaterialResourceByCloning(parentMaterialResourceId);

		{ // Set compositor resource pass quad material properties
			const MaterialProperties::SortedPropertyVector& sortedPropertyVector = static_cast<const CompositorResourcePassQuad&>(getCompositorResourcePass()).getMaterialProperties().getSortedPropertyVector();
			if (!sortedPropertyVector.empty())
			{
				MaterialResource& materialResource = materialResourceManager.getById(mMaterialResourceId);
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
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, Renderer::IVertexArrayPtr(), false, 0, 3, materialResourceManager, mMaterialResourceId, getInvalid<SkeletonResourceId>());
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
