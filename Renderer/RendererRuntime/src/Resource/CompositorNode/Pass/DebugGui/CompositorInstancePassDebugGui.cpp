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
#include "RendererRuntime/PrecompiledHeader.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/DebugGui/CompositorInstancePassDebugGui.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h"
#include "RendererRuntime/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorContextData.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/DebugGui/DebugGuiManager.h"
#include "RendererRuntime/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassDebugGui::onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		// Begin debug event
		COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(commandBuffer)

		// Fill command buffer
		DebugGuiManager& debugGuiManager = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getDebugGuiManager();
		RenderableManager::Renderables& renderables = mRenderableManager.getRenderables();
		if (renderables.empty())
		{
			// Fill command buffer using fixed build in renderer configuration resources
			compositorContextData.resetCurrentlyBoundMaterialBlueprintResource();
			debugGuiManager.fillCommandBufferUsingFixedBuildInRendererConfiguration(commandBuffer);
		}
		else
		{
			// Fill command buffer, this sets the material resource blueprint
			{
				Renderer::IVertexArrayPtr vertexArrayPtr = debugGuiManager.getFillVertexArrayPtr();
				if (vertexArrayPtr != renderables[0].getVertexArrayPtr())
				{
					renderables[0].setVertexArrayPtr(vertexArrayPtr);
				}
			}
			mRenderQueue.addRenderablesFromRenderableManager(mRenderableManager);
			if (mRenderQueue.getNumberOfDrawCalls() > 0)
			{
				mRenderQueue.fillCommandBuffer(renderTarget, static_cast<const CompositorResourcePassDebugGui&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData, commandBuffer);
			}

			// Fill command buffer using custom material blueprint resource
			debugGuiManager.fillCommandBuffer(commandBuffer);
		}

		// End debug event
		COMMAND_END_DEBUG_EVENT(commandBuffer)
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::CompositorInstancePassQuad methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassDebugGui::createMaterialResource(MaterialResourceId parentMaterialResourceId)
	{
		// Call the base implementation
		CompositorInstancePassQuad::createMaterialResource(parentMaterialResourceId);

		// Inside this compositor pass implementation, the renderable only exists to set the material blueprint
		mRenderableManager.getRenderables()[0].setNumberOfIndices(0);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassDebugGui::CompositorInstancePassDebugGui(const CompositorResourcePassDebugGui& compositorResourcePassDebugGui, const CompositorNodeInstance& compositorNodeInstance) :
		CompositorInstancePassQuad(compositorResourcePassDebugGui, compositorNodeInstance)
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
} // RendererRuntime
