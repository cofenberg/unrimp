/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Renderer/Public/Resource/CompositorNode/Pass/Scene/CompositorInstancePassScene.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
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
	void CompositorInstancePassScene::onCompositorWorkspaceInstanceLoadingFinished()
	{
		// Cache render queue index range instance, we know it must exist when we're in here
		mRenderQueueIndexRange = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderQueueIndexRangeByRenderQueueIndex(mRenderQueue.getMinimumRenderQueueIndex());
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != mRenderQueueIndexRange, "Invalid render queue index range")
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), mRenderQueueIndexRange->minimumRenderQueueIndex <= mRenderQueue.getMinimumRenderQueueIndex(), "Invalid minimum render queue index")
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), mRenderQueueIndexRange->maximumRenderQueueIndex >= mRenderQueue.getMaximumRenderQueueIndex(), "Invalid maximum render queue index")
	}

	void CompositorInstancePassScene::onFillCommandBuffer(const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != renderTarget, "The scene compositor instance pass needs a valid render target")

		// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
		RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), commandBuffer, getCompositorResourcePass().getDebugName())

		// Fill command buffer
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != mRenderQueueIndexRange, "Invalid render queue index range")
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
	CompositorInstancePassScene::CompositorInstancePassScene(const CompositorResourcePassScene& compositorResourcePassScene, const CompositorNodeInstance& compositorNodeInstance, bool positionOnlyPass) :
		ICompositorInstancePass(compositorResourcePassScene, compositorNodeInstance),
		mRenderQueue(compositorNodeInstance.getCompositorWorkspaceInstance().getRenderer().getMaterialBlueprintResourceManager().getIndirectBufferManager(), compositorResourcePassScene.getMinimumRenderQueueIndex(), compositorResourcePassScene.getMaximumRenderQueueIndex(), positionOnlyPass, compositorResourcePassScene.isTransparentPass(), true),
		mRenderQueueIndexRange(nullptr)
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
