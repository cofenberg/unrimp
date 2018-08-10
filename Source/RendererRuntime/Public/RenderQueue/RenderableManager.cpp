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
#include "RendererRuntime/Public/RenderQueue/RenderableManager.h"
#include "RendererRuntime/Public/Core/Math/Transform.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		const RendererRuntime::Transform IdentityTransform;


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	RenderableManager::RenderableManager() :
		mTransform(&::detail::IdentityTransform),
		mVisible(true),
		mCachedDistanceToCamera(getInvalid<float>()),
		mMinimumRenderQueueIndex(0),
		mMaximumRenderQueueIndex(0),
		mCastShadows(false)
	{
		// Nothing here
	}

	void RenderableManager::setTransform(const Transform* transform)
	{
		mTransform = (nullptr != transform) ? transform : &::detail::IdentityTransform;
	}

	void RenderableManager::updateCachedRenderablesData()
	{
		if (mRenderables.empty())
		{
			mMinimumRenderQueueIndex = 0;
			mMaximumRenderQueueIndex = 0;
			mCastShadows			 = false;
		}
		else
		{
			{ // Initialize the data using the first renderable
				const Renderable& renderable = mRenderables[0];
				mMinimumRenderQueueIndex = renderable.getRenderQueueIndex();
				mMaximumRenderQueueIndex = mMinimumRenderQueueIndex;
				mCastShadows			 = renderable.getCastShadows();
			}

			// Now incorporate the data from the other renderables
			const size_t numberOfRenderables = mRenderables.size();
			for (size_t i = 1; i < numberOfRenderables; ++i)
			{
				const Renderable& renderable = mRenderables[i];
				const uint8_t renderQueueIndex = renderable.getRenderQueueIndex();
				if (mMinimumRenderQueueIndex > renderQueueIndex)
				{
					mMinimumRenderQueueIndex = renderQueueIndex;
				}
				else if (mMaximumRenderQueueIndex < renderQueueIndex)
				{
					mMaximumRenderQueueIndex = renderQueueIndex;
				}
				if (renderable.getCastShadows())
				{
					mCastShadows = true;
				}
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
