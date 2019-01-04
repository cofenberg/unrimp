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
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeResource.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeResourceManager.h"
#include "RendererRuntime/Public/Resource/ResourceStreamer.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void CompositorNodeResource::enforceFullyLoaded()
	{
		// TODO(co) Implement more efficient solution: We need to extend "Runtime::ResourceStreamer" to request emergency immediate processing of requested resources
		ResourceStreamer& resourceStreamer = getResourceManager<CompositorNodeResourceManager>().getRendererRuntime().getResourceStreamer();
		while (IResource::LoadingState::LOADED != getLoadingState())
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1ms);
			resourceStreamer.dispatch();
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void CompositorNodeResource::deinitializeElement()
	{
		mInputChannels.clear();
		mCompositorRenderTargetTextures.clear();
		mCompositorFramebuffers.clear();
		mCompositorTargets.clear();
		mOutputChannels.clear();

		// Call base implementation
		IResource::deinitializeElement();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
