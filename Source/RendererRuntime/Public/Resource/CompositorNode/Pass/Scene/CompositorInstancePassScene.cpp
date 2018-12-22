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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Scene/CompositorInstancePassScene.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Core/IProfiler.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassScene::onCompositorWorkspaceInstanceLoadingFinished()
	{
		// Cache render queue index range instance, we know it must exist when we're in here
		mRenderQueueIndexRange = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderQueueIndexRangeByRenderQueueIndex(mRenderQueue.getMinimumRenderQueueIndex());
		RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), nullptr != mRenderQueueIndexRange, "Invalid render queue index range")
		RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), mRenderQueueIndexRange->minimumRenderQueueIndex <= mRenderQueue.getMinimumRenderQueueIndex(), "Invalid minimum render queue index")
		RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), mRenderQueueIndexRange->maximumRenderQueueIndex >= mRenderQueue.getMaximumRenderQueueIndex(), "Invalid maximum render queue index")
	}

	void CompositorInstancePassScene::onFillCommandBuffer(const Renderer::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), nullptr != renderTarget, "The scene compositor instance pass needs a valid render target")

		// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
		RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), commandBuffer, getCompositorResourcePass().getDebugName())

		// Fill command buffer
		RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), nullptr != mRenderQueueIndexRange, "Invalid render queue index range")
		const MaterialTechniqueId materialTechniqueId = static_cast<const CompositorResourcePassScene&>(getCompositorResourcePass()).getMaterialTechniqueId();
		for (const RenderableManager* renderableManager : mRenderQueueIndexRange->renderableManagers)
		{
			// The render queue index range covered by this compositor instance pass scene might be smaller than the range of the
			// cached render queue index range. So, we could add a range check in here to reject renderable managers, but it's not
			// really worth to do so since the render queue only considers renderables inside the render queue range anyway.
			mRenderQueue.addRenderablesFromRenderableManager(*renderableManager, materialTechniqueId, compositorContextData);
		}
		if (mRenderQueue.getNumberOfDrawCalls() > 0)
		{
			mRenderQueue.fillGraphicsCommandBuffer(*renderTarget, compositorContextData, commandBuffer);
		}
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	CompositorInstancePassScene::CompositorInstancePassScene(const CompositorResourcePassScene& compositorResourcePassScene, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassScene, compositorNodeInstance),
		mRenderQueue(compositorNodeInstance.getCompositorWorkspaceInstance().getRendererRuntime().getMaterialBlueprintResourceManager().getIndirectBufferManager(), compositorResourcePassScene.getMinimumRenderQueueIndex(), compositorResourcePassScene.getMaximumRenderQueueIndex(), compositorResourcePassScene.isTransparentPass(), true),
		mRenderQueueIndexRange(nullptr)
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
