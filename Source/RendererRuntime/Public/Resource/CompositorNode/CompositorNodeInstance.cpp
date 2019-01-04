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
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorTarget.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ICompositorInstancePass.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ICompositorResourcePass.h"

#include <Renderer/Public/Renderer.h>

#include <limits>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	CompositorNodeInstance::~CompositorNodeInstance()
	{
		for (ICompositorInstancePass* compositorInstancePass : mCompositorInstancePasses)
		{
			delete compositorInstancePass;
		}
	}

	void CompositorNodeInstance::compositorWorkspaceInstanceLoadingFinished() const
	{
		for (ICompositorInstancePass* compositorInstancePass : mCompositorInstancePasses)
		{
			compositorInstancePass->onCompositorWorkspaceInstanceLoadingFinished();
		}
	}

	Renderer::IRenderTarget& CompositorNodeInstance::fillCommandBuffer(Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer) const
	{
		Renderer::IRenderTarget* currentRenderTarget = nullptr;
		for (ICompositorInstancePass* compositorInstancePass : mCompositorInstancePasses)
		{
			// Check whether or not to execute the compositor pass instance
			const ICompositorResourcePass& compositorResourcePass = compositorInstancePass->getCompositorResourcePass();
			if ((!compositorResourcePass.getSkipFirstExecution() || compositorInstancePass->mNumberOfExecutionRequests > 0) &&
				(isInvalid(compositorResourcePass.getNumberOfExecutions()) || compositorInstancePass->mNumberOfExecutionRequests < compositorResourcePass.getNumberOfExecutions()))
			{
				{ // Set the current graphics render target
					// TODO(co) For now: In case if it's a compositor channel ID (input/output node) use the given render target
					Renderer::IRenderTarget* newRenderTarget = isValid(compositorResourcePass.getCompositorTarget().getCompositorChannelId()) ? &renderTarget : compositorInstancePass->getRenderTarget();
					if (newRenderTarget != currentRenderTarget)
					{
						currentRenderTarget = newRenderTarget;
						Renderer::Command::SetGraphicsRenderTarget::create(commandBuffer, currentRenderTarget);
					}

					// Set the graphics viewport and scissor rectangle
					// -> Can't be moved into the render target change branch above since a compositor resource pass might e.g. change the minimum depth
					if (nullptr != currentRenderTarget)
					{
						// Get the window size
						uint32_t width  = 1;
						uint32_t height = 1;
						currentRenderTarget->getWidthAndHeight(width, height);

						// Set the graphics viewport and scissor rectangle
						Renderer::Command::SetGraphicsViewportAndScissorRectangle::create(commandBuffer, 0, 0, width, height, compositorResourcePass.getMinimumDepth(), compositorResourcePass.getMaximumDepth());
					}
				}

				// Let the compositor instance pass fill the command buffer
				compositorInstancePass->onFillCommandBuffer(currentRenderTarget, compositorContextData, commandBuffer);
			}

			// Update the number of compositor instance pass execution requests and don't forget to avoid integer range overflow
			if (compositorInstancePass->mNumberOfExecutionRequests < std::numeric_limits<uint32_t>::max())
			{
				++compositorInstancePass->mNumberOfExecutionRequests;
			}
		}

		// Sanity check: At least for now a compositor node must end with a valid current render target
		assert(nullptr != currentRenderTarget);

		// Done
		return *currentRenderTarget;
	}

	void CompositorNodeInstance::onPostCommandBufferExecution() const
	{
		for (ICompositorInstancePass* compositorInstancePass : mCompositorInstancePasses)
		{
			compositorInstancePass->onPostCommandBufferExecution();
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
