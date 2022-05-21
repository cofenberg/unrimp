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
#include "Examples/Private/Renderer/Compositor/CompositorInstancePass.h"
#include "Examples/Private/Renderer/Compositor/CompositorResourcePass.h"

#include <Renderer/Public/IRenderer.h>
#ifdef RENDERER_IMGUI
	#include <Renderer/Public/DebugGui/DebugGuiManager.h>
	#include <Renderer/Public/DebugGui/DebugGuiHelper.h>
#endif
#include <Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h>
#include <Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h>


//[-------------------------------------------------------]
//[ Protected virtual Renderer::ICompositorInstancePass methods ]
//[-------------------------------------------------------]
void CompositorInstancePass::onFillCommandBuffer([[maybe_unused]] const Rhi::IRenderTarget* renderTarget, const Renderer::CompositorContextData&, [[maybe_unused]] Rhi::CommandBuffer& commandBuffer)
{
	// Sanity check
	RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != renderTarget, "The example compositor instance pass needs a valid render target")

	// Well right now I'm not that creative and the purpose of this example is to show how to add custom compositor passes, so, draw a simple text
	#ifdef RENDERER_IMGUI
		const Renderer::CompositorWorkspaceInstance& compositorWorkspaceInstance = getCompositorNodeInstance().getCompositorWorkspaceInstance();
		Renderer::DebugGuiManager& debugGuiManager = compositorWorkspaceInstance.getRenderer().getDebugGuiManager();
		debugGuiManager.newFrame(*compositorWorkspaceInstance.getExecutionRenderTarget());	// We know that the render target must be valid if we're in here
		Renderer::DebugGuiHelper::drawText("42", 100.0f, 100.0f);
		debugGuiManager.fillGraphicsCommandBufferUsingFixedBuildInRhiConfiguration(commandBuffer);
	#else
		RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), false, "ImGui support is disabled")
	#endif
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
CompositorInstancePass::CompositorInstancePass(const CompositorResourcePass& compositorResourcePass, const Renderer::CompositorNodeInstance& compositorNodeInstance) :
	ICompositorInstancePass(compositorResourcePass, compositorNodeInstance)
{
	// Nothing here
}
