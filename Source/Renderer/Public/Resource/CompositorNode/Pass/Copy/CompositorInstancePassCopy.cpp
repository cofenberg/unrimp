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
#include "Renderer/Public/Resource/CompositorNode/Pass/Copy/CompositorInstancePassCopy.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/Copy/CompositorResourcePassCopy.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "Renderer/Public/Resource/Texture/TextureResourceManager.h"
#include "Renderer/Public/Resource/Texture/TextureResource.h"
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
	void CompositorInstancePassCopy::onFillCommandBuffer([[maybe_unused]] const Rhi::IRenderTarget* renderTarget, const CompositorContextData&, Rhi::CommandBuffer& commandBuffer)
	{
		const CompositorResourcePassCopy& compositorResourcePassCopy = static_cast<const CompositorResourcePassCopy&>(getCompositorResourcePass());
		const IRenderer& renderer = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer();

		// Sanity check
		RHI_ASSERT(renderer.getContext(), nullptr == renderTarget, "The copy compositor instance pass needs an invalid render target")

		// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
		RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(renderer.getContext(), commandBuffer, compositorResourcePassCopy.getDebugName())

		// Get destination and source texture resources
		// TODO(co) "Renderer::TextureResourceManager::getTextureResourceByAssetId()" is considered to be inefficient, don't use it in here
		const TextureResourceManager& textureResourceManager = renderer.getTextureResourceManager();
		const TextureResource* destinationTextureResource = textureResourceManager.getTextureResourceByAssetId(compositorResourcePassCopy.getDestinationTextureAssetId());
		const TextureResource* sourceTextureResource = textureResourceManager.getTextureResourceByAssetId(compositorResourcePassCopy.getSourceTextureAssetId());
		if (nullptr != destinationTextureResource && nullptr != sourceTextureResource)
		{
			const Rhi::ITexturePtr& destinationTexturePtr = destinationTextureResource->getTexturePtr();
			const Rhi::ITexturePtr& sourceTexturePtr = sourceTextureResource->getTexturePtr();
			if (nullptr != destinationTexturePtr && nullptr != sourceTexturePtr)
			{
				Rhi::Command::CopyResource::create(commandBuffer, *destinationTexturePtr, *sourceTexturePtr);
			}
			else
			{
				// Error!
				RHI_ASSERT(renderer.getContext(), false, "We should never end up in here")
			}
		}
		else
		{
			// Error!
			RHI_ASSERT(renderer.getContext(), false, "We should never end up in here")
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassCopy::CompositorInstancePassCopy(const CompositorResourcePassCopy& compositorResourcePassCopy, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassCopy, compositorNodeInstance)
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
