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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ResolveMultisample/CompositorInstancePassResolveMultisample.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ResolveMultisample/CompositorResourcePassResolveMultisample.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/Public/Core/Renderer/FramebufferManager.h"
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
	void CompositorInstancePassResolveMultisample::onFillCommandBuffer(const Renderer::IRenderTarget* renderTarget, const CompositorContextData&, Renderer::CommandBuffer& commandBuffer)
	{
		const IRendererRuntime& rendererRuntime = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime();

		// Sanity check
		RENDERER_ASSERT(rendererRuntime.getContext(), nullptr != renderTarget, "The resolve multisample compositor instance pass needs a valid render target")

		// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
		RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(rendererRuntime.getContext(), commandBuffer, getCompositorResourcePass().getDebugName())

		// Resolve
		Renderer::IFramebuffer* framebuffer = rendererRuntime.getCompositorWorkspaceResourceManager().getFramebufferManager().getFramebufferByCompositorFramebufferId(static_cast<const CompositorResourcePassResolveMultisample&>(getCompositorResourcePass()).getSourceMultisampleCompositorFramebufferId());
		if (nullptr != framebuffer)
		{
			// TODO(co) Get rid of the evil const-cast
			Renderer::Command::ResolveMultisampleFramebuffer::create(commandBuffer, const_cast<Renderer::IRenderTarget&>(*renderTarget), *framebuffer);
		}
		else
		{
			// Error!
			RENDERER_ASSERT(rendererRuntime.getContext(), false, "We should never end up in here")
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassResolveMultisample::CompositorInstancePassResolveMultisample(const CompositorResourcePassResolveMultisample& compositorResourcePassResolveMultisample, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassResolveMultisample, compositorNodeInstance)
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
