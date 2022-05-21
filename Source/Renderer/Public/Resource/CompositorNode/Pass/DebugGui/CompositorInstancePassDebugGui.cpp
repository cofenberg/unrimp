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
#include "Renderer/Public/Resource/CompositorNode/Pass/DebugGui/CompositorInstancePassDebugGui.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorContextData.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "Renderer/Public/DebugGui/DebugGuiManager.h"
#include "Renderer/Public/Core/IProfiler.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassDebugGui::onFillCommandBuffer([[maybe_unused]] const Rhi::IRenderTarget* renderTarget, [[maybe_unused]] const CompositorContextData& compositorContextData, [[maybe_unused]] Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != renderTarget, "The debug GUI compositor instance pass needs a valid render target")

		#ifdef RENDERER_IMGUI
			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			const IRenderer& renderer = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer();
			RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(renderer.getContext(), commandBuffer, getCompositorResourcePass().getDebugName())

			// Fill command buffer
			DebugGuiManager& debugGuiManager = renderer.getDebugGuiManager();
			RenderableManager::Renderables& renderables = mRenderableManager.getRenderables();
			compositorContextData.resetCurrentlyBoundMaterialBlueprintResource();
			if (renderables.empty())
			{
				// Fill command buffer using fixed build in RHI configuration resources
				debugGuiManager.fillGraphicsCommandBufferUsingFixedBuildInRhiConfiguration(commandBuffer);
			}
			else
			{
				// Fill command buffer, this sets the material resource blueprint
				{
					const Rhi::IVertexArrayPtr& vertexArrayPtr = debugGuiManager.getFillVertexArrayPtr();
					if (vertexArrayPtr != renderables[0].getVertexArrayPtr())
					{
						renderables[0].setVertexArrayPtr(vertexArrayPtr);
					}
				}
				mRenderQueue.addRenderablesFromRenderableManager(mRenderableManager, static_cast<const CompositorResourcePassDebugGui&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData);
				if (mRenderQueue.getNumberOfDrawCalls() > 0)
				{
					mRenderQueue.fillGraphicsCommandBuffer(*renderTarget, compositorContextData, commandBuffer);

					// Fill command buffer using custom graphics material blueprint resource
					if (nullptr != compositorContextData.getCurrentlyBoundMaterialBlueprintResource())
					{
						debugGuiManager.fillGraphicsCommandBuffer(commandBuffer);
					}
				}
			}
		#else
			RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), false, "ImGui support is disabled")
		#endif
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::CompositorInstancePassCompute methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassDebugGui::createMaterialResource(MaterialResourceId parentMaterialResourceId)
	{
		// Call the base implementation
		CompositorInstancePassCompute::createMaterialResource(parentMaterialResourceId);

		// Inside this compositor pass implementation, the renderable only exists to set the material blueprint
		mRenderableManager.getRenderables()[0].setNumberOfIndices(0);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassDebugGui::CompositorInstancePassDebugGui(const CompositorResourcePassDebugGui& compositorResourcePassDebugGui, const CompositorNodeInstance& compositorNodeInstance) :
		CompositorInstancePassCompute(compositorResourcePassDebugGui, compositorNodeInstance)
	{
		// Inside this compositor pass implementation, the renderable only exists to set the material blueprint
		RenderableManager::Renderables& renderables = mRenderableManager.getRenderables();
		if (!renderables.empty())
		{
			renderables[0].setNumberOfIndices(0);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
