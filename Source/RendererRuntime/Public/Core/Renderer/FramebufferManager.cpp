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
#include "RendererRuntime/Public/Core/Renderer/FramebufferManager.h"
#include "RendererRuntime/Public/Core/Renderer/RenderPassManager.h"
#include "RendererRuntime/Public/Core/Renderer/RenderTargetTextureManager.h"
#include "RendererRuntime/Public/IRendererRuntime.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline bool orderFramebufferElementByFramebufferSignatureId(const RendererRuntime::FramebufferManager::FramebufferElement& left, const RendererRuntime::FramebufferManager::FramebufferElement& right)
		{
			return (left.framebufferSignature.getFramebufferSignatureId() < right.framebufferSignature.getFramebufferSignatureId());
		}


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
	void FramebufferManager::clear()
	{
		clearRhiResources();
		mSortedFramebufferVector.clear();
		mCompositorFramebufferIdToFramebufferSignatureId.clear();
	}

	void FramebufferManager::clearRhiResources()
	{
		for (FramebufferElement& framebufferElement : mSortedFramebufferVector)
		{
			if (nullptr != framebufferElement.framebuffer)
			{
				framebufferElement.framebuffer->releaseReference();
				framebufferElement.framebuffer = nullptr;
			}
		}
	}

	void FramebufferManager::addFramebuffer(CompositorFramebufferId compositorFramebufferId, const FramebufferSignature& framebufferSignature)
	{
		FramebufferElement framebufferElement(framebufferSignature);
		SortedFramebufferVector::iterator iterator = std::lower_bound(mSortedFramebufferVector.begin(), mSortedFramebufferVector.end(), framebufferElement, ::detail::orderFramebufferElementByFramebufferSignatureId);
		if (iterator == mSortedFramebufferVector.end() || iterator->framebufferSignature.getFramebufferSignatureId() != framebufferElement.framebufferSignature.getFramebufferSignatureId())
		{
			// Add new framebuffer

			// Register the new framebuffer element
			++framebufferElement.numberOfReferences;
			mSortedFramebufferVector.insert(iterator, framebufferElement);
		}
		else
		{
			// Just increase the number of references
			++iterator->numberOfReferences;
		}
		mCompositorFramebufferIdToFramebufferSignatureId.emplace(compositorFramebufferId, framebufferSignature.getFramebufferSignatureId());
	}

	Rhi::IFramebuffer* FramebufferManager::getFramebufferByCompositorFramebufferId(CompositorFramebufferId compositorFramebufferId) const
	{
		Rhi::IFramebuffer* framebuffer = nullptr;

		// Map compositor framebuffer ID to framebuffer signature ID
		CompositorFramebufferIdToFramebufferSignatureId::const_iterator iterator = mCompositorFramebufferIdToFramebufferSignatureId.find(compositorFramebufferId);
		if (mCompositorFramebufferIdToFramebufferSignatureId.cend() != iterator)
		{
			// TODO(co) Is there need for a more efficient search?
			const FramebufferSignatureId framebufferSignatureId = iterator->second;
			for (const FramebufferElement& framebufferElement : mSortedFramebufferVector)
			{
				const FramebufferSignature& framebufferSignature = framebufferElement.framebufferSignature;
				if (framebufferSignature.getFramebufferSignatureId() == framebufferSignatureId)
				{
					framebuffer = framebufferElement.framebuffer;
					break;
				}
			}
			ASSERT(nullptr != framebuffer);
		}
		else
		{
			// Error! Unknown compositor framebuffer ID, this shouldn't have happened.
			ASSERT(false);
		}

		// Done
		return framebuffer;
	}

	Rhi::IFramebuffer* FramebufferManager::getFramebufferByCompositorFramebufferId(CompositorFramebufferId compositorFramebufferId, const Rhi::IRenderTarget& renderTarget, uint8_t numberOfMultisamples, float resolutionScale)
	{
		Rhi::IFramebuffer* framebuffer = nullptr;

		// Map compositor framebuffer ID to framebuffer signature ID
		CompositorFramebufferIdToFramebufferSignatureId::const_iterator iterator = mCompositorFramebufferIdToFramebufferSignatureId.find(compositorFramebufferId);
		if (mCompositorFramebufferIdToFramebufferSignatureId.cend() != iterator)
		{
			// TODO(co) Is there need for a more efficient search?
			const FramebufferSignatureId framebufferSignatureId = iterator->second;
			for (FramebufferElement& framebufferElement : mSortedFramebufferVector)
			{
				const FramebufferSignature& framebufferSignature = framebufferElement.framebufferSignature;
				if (framebufferSignature.getFramebufferSignatureId() == framebufferSignatureId)
				{
					// Do we need to create the RHI framebuffer instance right now?
					if (nullptr == framebufferElement.framebuffer)
					{
						// Get the color texture instances
						Rhi::TextureFormat::Enum colorTextureFormats[8] = { Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN, Rhi::TextureFormat::Enum::UNKNOWN };
						const uint8_t numberOfColorAttachments = framebufferSignature.getNumberOfColorAttachments();
						ASSERT(numberOfColorAttachments < 8);
						Rhi::FramebufferAttachment colorFramebufferAttachments[8];
						uint8_t usedNumberOfMultisamples = 0;
						for (uint8_t i = 0; i < numberOfColorAttachments; ++i)
						{
							const FramebufferSignatureAttachment& framebufferSignatureAttachment = framebufferSignature.getColorFramebufferSignatureAttachment(i);
							const AssetId colorTextureAssetId = framebufferSignatureAttachment.textureAssetId;
							const RenderTargetTextureSignature* colorRenderTargetTextureSignature = nullptr;
							Rhi::FramebufferAttachment& framebufferAttachment = colorFramebufferAttachments[i];
							framebufferAttachment.texture = isValid(colorTextureAssetId) ? mRenderTargetTextureManager.getTextureByAssetId(colorTextureAssetId, renderTarget, numberOfMultisamples, resolutionScale, &colorRenderTargetTextureSignature) : nullptr;
							ASSERT(nullptr != framebufferAttachment.texture);
							framebufferAttachment.mipmapIndex = framebufferSignatureAttachment.mipmapIndex;
							framebufferAttachment.layerIndex = framebufferSignatureAttachment.layerIndex;
							ASSERT(nullptr != colorRenderTargetTextureSignature);
							if (0 == usedNumberOfMultisamples)
							{
								usedNumberOfMultisamples = ((colorRenderTargetTextureSignature->getFlags() & RenderTargetTextureSignature::Flag::ALLOW_MULTISAMPLE) != 0) ? numberOfMultisamples : 1u;
							}
							else
							{
								ASSERT(1 == usedNumberOfMultisamples || ((colorRenderTargetTextureSignature->getFlags() & RenderTargetTextureSignature::Flag::ALLOW_MULTISAMPLE) != 0));
							}
							colorTextureFormats[i] = colorRenderTargetTextureSignature->getTextureFormat();
						}

						// Get the depth stencil texture instances
						const FramebufferSignatureAttachment& depthStencilFramebufferSignatureAttachment = framebufferSignature.getDepthStencilFramebufferSignatureAttachment();
						const RenderTargetTextureSignature* depthStencilRenderTargetTextureSignature = nullptr;
						Rhi::FramebufferAttachment depthStencilFramebufferAttachment(isValid(depthStencilFramebufferSignatureAttachment.textureAssetId) ? mRenderTargetTextureManager.getTextureByAssetId(depthStencilFramebufferSignatureAttachment.textureAssetId, renderTarget, numberOfMultisamples, resolutionScale, &depthStencilRenderTargetTextureSignature) : nullptr, depthStencilFramebufferSignatureAttachment.mipmapIndex, depthStencilFramebufferSignatureAttachment.layerIndex);
						if (nullptr != depthStencilRenderTargetTextureSignature)
						{
							if (0 == usedNumberOfMultisamples)
							{
								usedNumberOfMultisamples = ((depthStencilRenderTargetTextureSignature->getFlags() & RenderTargetTextureSignature::Flag::ALLOW_MULTISAMPLE) != 0) ? numberOfMultisamples : 1u;
							}
							else
							{
								ASSERT(1 == usedNumberOfMultisamples || ((depthStencilRenderTargetTextureSignature->getFlags() & RenderTargetTextureSignature::Flag::ALLOW_MULTISAMPLE) != 0));
							}
						}
						const Rhi::TextureFormat::Enum depthStencilTextureFormat = (nullptr != depthStencilRenderTargetTextureSignature) ? depthStencilRenderTargetTextureSignature->getTextureFormat() : Rhi::TextureFormat::Enum::UNKNOWN;

						// Get or create the managed render pass
						Rhi::IRenderPass* renderPass = mRenderPassManager.getOrCreateRenderPass(numberOfColorAttachments, colorTextureFormats, depthStencilTextureFormat, usedNumberOfMultisamples);
						ASSERT(nullptr != renderPass);

						// Create the framebuffer object (FBO) instance
						// -> The framebuffer automatically adds a reference to the provided textures
						framebufferElement.framebuffer = mRenderTargetTextureManager.getRendererRuntime().getRhi().createFramebuffer(*renderPass, colorFramebufferAttachments, (nullptr != depthStencilFramebufferAttachment.texture) ? &depthStencilFramebufferAttachment : nullptr);
						RHI_SET_RESOURCE_DEBUG_NAME(framebufferElement.framebuffer, "Framebuffer manager")
						framebufferElement.framebuffer->addReference();
					}
					framebuffer = framebufferElement.framebuffer;
					break;
				}
			}
			ASSERT(nullptr != framebuffer);
		}
		else
		{
			// Error! Unknown compositor framebuffer ID, this shouldn't have happened.
			ASSERT(false);
		}

		// Done
		return framebuffer;
	}

	void FramebufferManager::releaseFramebufferBySignature(const FramebufferSignature& framebufferSignature)
	{
		const FramebufferElement framebufferElement(framebufferSignature);
		SortedFramebufferVector::iterator iterator = std::lower_bound(mSortedFramebufferVector.begin(), mSortedFramebufferVector.end(), framebufferElement, ::detail::orderFramebufferElementByFramebufferSignatureId);
		if (iterator != mSortedFramebufferVector.end() && iterator->framebufferSignature.getFramebufferSignatureId() == framebufferElement.framebufferSignature.getFramebufferSignatureId())
		{
			// Was this the last reference?
			if (1 == iterator->numberOfReferences)
			{
				if (nullptr != iterator->framebuffer)
				{
					iterator->framebuffer->releaseReference();
				}
				mSortedFramebufferVector.erase(iterator);
			}
			else
			{
				--iterator->numberOfReferences;
			}
		}
		else
		{
			// Error! Framebuffer signature isn't registered.
			ASSERT(false);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
