/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Renderer/Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorInstancePassGenerateMipmaps.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/Compute/CompositorInstancePassCompute.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorContextData.h"
#include "Renderer/Public/Resource/Material/MaterialResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialResource.h"
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
	void CompositorInstancePassGenerateMipmaps::onFillCommandBuffer([[maybe_unused]] const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer)
	{
		const IRenderer& renderer = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer();

		// Sanity check
		RHI_ASSERT(renderer.getContext(), nullptr == renderTarget, "The generate mipmaps compositor instance pass needs an invalid render target")

		// Handle texture mipmap generation via custom material blueprint
		const CompositorResourcePassGenerateMipmaps& compositorResourcePassGenerateMipmaps = static_cast<const CompositorResourcePassGenerateMipmaps&>(getCompositorResourcePass());
		RHI_ASSERT(renderer.getContext(), isValid(compositorResourcePassGenerateMipmaps.getTextureAssetId()), "Invalid compositor resource pass generate mipmaps texture asset ID")
		const AssetId materialBlueprintAssetId = compositorResourcePassGenerateMipmaps.getMaterialBlueprintAssetId();
		if (isValid(materialBlueprintAssetId))
		{
			// Sanity check
			RHI_ASSERT(renderer.getContext(), isValid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")

			{ // Record reusable command buffer, if necessary
				const TextureResourceManager& textureResourceManager = renderer.getTextureResourceManager();
				// TODO(co) "Renderer::TextureResourceManager::getTextureResourceByAssetId()" is considered to be inefficient, don't use it in here
				TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(compositorResourcePassGenerateMipmaps.getTextureAssetId());
				if (nullptr != textureResource)
				{
					Rhi::ITexture* texture = textureResource->getTexturePtr();
					if (nullptr != texture)
					{
						// Sanity check
						RHI_ASSERT(renderer.getContext(), texture->getResourceType() == Rhi::ResourceType::TEXTURE_2D, "The generate mipmaps compositor instance pass needs an 2D texture as texture")

						// Render target size changed?
						Rhi::ITexture2D* texture2D = static_cast<Rhi::ITexture2D*>(texture);
						const uint32_t renderTargetWidth = texture2D->getWidth();
						const uint32_t renderTargetHeight = texture2D->getHeight();
						const uint32_t numberOfMipmaps = Rhi::ITexture::getNumberOfMipmaps(renderTargetWidth, renderTargetHeight);
						if (mRenderTargetWidth != renderTargetWidth || mRenderTargetHeight != renderTargetHeight)
						{
							mRenderTargetWidth = renderTargetWidth;
							mRenderTargetHeight = renderTargetHeight;
							mFramebuffersPtrs.resize(numberOfMipmaps);
							Rhi::IRhi& rhi = renderer.getRhi();
							Rhi::IRenderPass* renderPass = rhi.createRenderPass(0, nullptr, Rhi::TextureFormat::D32_FLOAT, 1 RHI_RESOURCE_DEBUG_NAME("Compositor instance pass generate mipmap"));	// TODO(co) Make the texture format flexible, custom mipmap generation also makes sense for color textures
							for (uint32_t mipmapIndex = 1; mipmapIndex < numberOfMipmaps; ++mipmapIndex)
							{
								const Rhi::FramebufferAttachment depthFramebufferAttachment(texture, mipmapIndex, 0);
								mFramebuffersPtrs[mipmapIndex] = rhi.createFramebuffer(*renderPass, nullptr, &depthFramebufferAttachment RHI_RESOURCE_DEBUG_NAME(("Compositor instance pass generate mipmap " + std::to_string(mipmapIndex)).c_str()));
							}
						}

						// Record reusable command buffer
						// TODO(co) Optimization: Make this hot-reloading ready and also listen to other critical compositor setting changes like number of multisamples, when done we can fill the following command buffer once and then just reuse it
						// TODO(co) There's certainly room for command buffer optimization here (e.g. the graphics pipeline state stays the same)
						mCommandBuffer.clear();
						if (!mFramebuffersPtrs.empty())
						{
							// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
							RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(renderer.getContext(), mCommandBuffer, compositorResourcePassGenerateMipmaps.getDebugName())

							// Basing on "Hierarchical-Z map based occlusion culling" - "Hi-Z map construction" - http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/
							uint32_t currentWidth = renderTargetWidth;
							uint32_t currentHeight = renderTargetHeight;
							for (uint32_t mipmapIndex = 1; mipmapIndex < numberOfMipmaps; ++mipmapIndex)
							{
								// Calculate next viewport size and ensure that the viewport size is always at least 1x1
								currentWidth = Rhi::ITexture::getHalfSize(currentWidth);
								currentHeight = Rhi::ITexture::getHalfSize(currentHeight);

								// Set graphics render target
								Rhi::Command::SetGraphicsRenderTarget::create(mCommandBuffer, mFramebuffersPtrs[mipmapIndex]);

								// Set the graphics viewport and scissor rectangle
								Rhi::Command::SetGraphicsViewportAndScissorRectangle::create(mCommandBuffer, 0, 0, currentWidth, currentHeight);

								// Restrict fetches only to previous depth texture mipmap level
								Rhi::Command::SetTextureMinimumMaximumMipmapIndex::create(mCommandBuffer, *texture, mipmapIndex - 1, mipmapIndex - 1);

								// Execute the compute pass
								CompositorContextData localCompositorContextData(compositorContextData.getCompositorWorkspaceInstance(), nullptr);
								mCompositorInstancePassCompute->onFillCommandBuffer(mFramebuffersPtrs[mipmapIndex], localCompositorContextData, mCommandBuffer);
								mCompositorInstancePassCompute->onPostCommandBufferDispatch();
							}

							// Reset mipmap level range for the depth texture
							Rhi::Command::SetTextureMinimumMaximumMipmapIndex::create(mCommandBuffer, *texture, 0, numberOfMipmaps - 1);
						}
					}
					else
					{
						// Error!
						RHI_ASSERT(renderer.getContext(), false, "Texture resource has no RHI texture instance")
					}
				}
				else
				{
					// Error!
					RHI_ASSERT(renderer.getContext(), false, "Failed to get texture resource by asset ID")
				}
			}

			// Fill given command buffer, if necessary
			if (!mCommandBuffer.isEmpty())
			{
				mCommandBuffer.appendToCommandBuffer(commandBuffer);
			}
		}
		else
		{
			// Sanity check
			RHI_ASSERT(renderer.getContext(), isInvalid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")

			// Generate mipmaps
			// TODO(co) "Renderer::TextureResourceManager::getTextureResourceByAssetId()" is considered to be inefficient, don't use it in here
			TextureResource* textureResource = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getTextureResourceManager().getTextureResourceByAssetId(compositorResourcePassGenerateMipmaps.getTextureAssetId());
			if (nullptr != textureResource)
			{
				Rhi::ITexture* texture = textureResource->getTexturePtr();
				if (nullptr != texture)
				{
					Rhi::Command::GenerateMipmaps::create(commandBuffer, *texture);
				}
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassGenerateMipmaps::CompositorInstancePassGenerateMipmaps(const CompositorResourcePassGenerateMipmaps& compositorResourcePassGenerateMipmaps, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassGenerateMipmaps, compositorNodeInstance),
		mCompositorResourcePassCompute(nullptr),
		mCompositorInstancePassCompute(nullptr),
		mRenderTargetWidth(getInvalid<uint32_t>()),
		mRenderTargetHeight(getInvalid<uint32_t>())
	{
		// Handle texture mipmap generation via custom material blueprint
		const AssetId materialBlueprintAssetId = compositorResourcePassGenerateMipmaps.getMaterialBlueprintAssetId();
		if (isValid(materialBlueprintAssetId))
		{
			// Sanity check
			RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), isValid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")

			// Create compositor pass compute
			MaterialProperties materialProperties;
			mCompositorResourcePassCompute = new CompositorResourcePassCompute(compositorResourcePassGenerateMipmaps.getCompositorTarget(), materialBlueprintAssetId, materialProperties);
			#if defined(RHI_DEBUG) || defined(RENDERER_PROFILER)
				mCompositorResourcePassCompute->setDebugName("Generate mipmap");
			#endif
			mCompositorInstancePassCompute = new CompositorInstancePassCompute(*mCompositorResourcePassCompute, getCompositorNodeInstance());
			getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getMaterialResourceManager().getById(mCompositorInstancePassCompute->getMaterialResourceId()).setPropertyById(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty(), MaterialPropertyValue::fromTextureAssetId(compositorResourcePassGenerateMipmaps.getTextureAssetId()));
		}
		else
		{
			// Sanity check
			RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), isInvalid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")
		}
	}

	CompositorInstancePassGenerateMipmaps::~CompositorInstancePassGenerateMipmaps()
	{
		// Handle texture mipmap generation via custom material blueprint: Destroy compositor pass compute, if there's one
		if (nullptr != mCompositorInstancePassCompute)
		{
			delete mCompositorInstancePassCompute;
			mCompositorInstancePassCompute = nullptr;
			RHI_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRenderer().getContext(), nullptr != mCompositorResourcePassCompute, "Invalid compositor resource pass compute")
			delete mCompositorResourcePassCompute;
			mCompositorResourcePassCompute = nullptr;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
