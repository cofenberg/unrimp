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
#include "RendererRuntime/Resource/CompositorNode/Pass/Scene/CompositorInstancePassScene.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.h"
#include "RendererRuntime/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/IRendererRuntime.h"


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
		assert(nullptr != mRenderQueueIndexRange);
		assert(mRenderQueueIndexRange->minimumRenderQueueIndex <= mRenderQueue.getMinimumRenderQueueIndex());
		assert(mRenderQueueIndexRange->maximumRenderQueueIndex >= mRenderQueue.getMaximumRenderQueueIndex());
	}

	void CompositorInstancePassScene::onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(commandBuffer)

		// Fill command buffer
		assert(nullptr != mRenderQueueIndexRange);
		for (const RenderableManager* renderableManager : mRenderQueueIndexRange->renderableManagers)
		{
			// The render queue index range covered by this compositor instance pass scene might be smaller than the range of the
			// cached render queue index range. So, we could add a range check in here to reject renderable managers, but it's not
			// really worth to do so since the render queue only considers renderables inside the render queue range anyway.
			mRenderQueue.addRenderablesFromRenderableManager(*renderableManager);
		}
		if (mRenderQueue.getNumberOfDrawCalls() > 0)
		{
			mRenderQueue.fillCommandBuffer(renderTarget, static_cast<const CompositorResourcePassScene&>(getCompositorResourcePass()).getMaterialTechniqueId(), compositorContextData, commandBuffer);
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
