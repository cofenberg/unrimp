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
#include "Examples/Private/Runtime/FirstCompositor/CompositorInstancePassFirst.h"
#include "Examples/Private/Runtime/FirstCompositor/CompositorResourcePassFirst.h"

#include <RendererRuntime/Public/IRendererRuntime.h>
#ifdef RENDERER_RUNTIME_IMGUI
	#include <RendererRuntime/Public/DebugGui/DebugGuiManager.h>
	#include <RendererRuntime/Public/DebugGui/DebugGuiHelper.h>
#endif
#include <RendererRuntime/Public/Resource/CompositorNode/CompositorNodeInstance.h>
#include <RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h>


//[-------------------------------------------------------]
//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
//[-------------------------------------------------------]
void CompositorInstancePassFirst::onFillCommandBuffer([[maybe_unused]] const Renderer::IRenderTarget* renderTarget, const RendererRuntime::CompositorContextData&, [[maybe_unused]] Renderer::CommandBuffer& commandBuffer)
{
	// Sanity check
	assert((nullptr != renderTarget) && "The first example compositor instance pass needs a valid render target");

	// Well right now I'm not that creative and the purpose of this example is to show how to add custom compositor passes, so, draw a simple text
	#ifdef RENDERER_RUNTIME_IMGUI
		const RendererRuntime::CompositorWorkspaceInstance& compositorWorkspaceInstance = getCompositorNodeInstance().getCompositorWorkspaceInstance();
		RendererRuntime::DebugGuiManager& debugGuiManager = compositorWorkspaceInstance.getRendererRuntime().getDebugGuiManager();
		debugGuiManager.newFrame(*compositorWorkspaceInstance.getExecutionRenderTarget());	// We know that the render target must be valid if we're in here
		RendererRuntime::DebugGuiHelper::drawText("42", 100.0f, 100.0f);
		debugGuiManager.fillGraphicsCommandBufferUsingFixedBuildInRendererConfiguration(commandBuffer);
	#else
		assert(false && "ImGui support is disabled");
	#endif
}


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
CompositorInstancePassFirst::CompositorInstancePassFirst(const CompositorResourcePassFirst& compositorResourcePassFirst, const RendererRuntime::CompositorNodeInstance& compositorNodeInstance) :
	ICompositorInstancePass(compositorResourcePassFirst, compositorNodeInstance)
{
	// Nothing here
}
