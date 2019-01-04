/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Loader/CompositorNodeFileFormat.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorResourcePass methods ]
	//[-------------------------------------------------------]
	void CompositorResourcePassScene::deserialize([[maybe_unused]] uint32_t numberOfBytes, const uint8_t* data)
	{
		// Sanity check
		assert(sizeof(v1CompositorNode::PassScene) == numberOfBytes);

		// Call the base implementation
		ICompositorResourcePass::deserialize(sizeof(v1CompositorNode::Pass), data);

		// Read data
		const v1CompositorNode::PassScene* passScene = reinterpret_cast<const v1CompositorNode::PassScene*>(data);
		mMinimumRenderQueueIndex = passScene->minimumRenderQueueIndex;
		mMaximumRenderQueueIndex = passScene->maximumRenderQueueIndex;
		mTransparentPass		 = passScene->transparentPass;
		mMaterialTechniqueId	 = passScene->materialTechniqueId;

		// Sanity check
		assert(mMaximumRenderQueueIndex >= mMinimumRenderQueueIndex);
	}

	bool CompositorResourcePassScene::getRenderQueueIndexRange(uint8_t& minimumRenderQueueIndex, uint8_t& maximumRenderQueueIndex) const
	{
		// This compositor resource pass has a render queue range defined
		minimumRenderQueueIndex = mMinimumRenderQueueIndex;
		maximumRenderQueueIndex = mMaximumRenderQueueIndex;
		return true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
