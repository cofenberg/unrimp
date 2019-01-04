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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorInstancePassGenerateMipmaps.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Compute/CompositorInstancePassCompute.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorContextData.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResource.h"
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
	void CompositorInstancePassGenerateMipmaps::onFillCommandBuffer([[maybe_unused]] const Renderer::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		const IRendererRuntime& rendererRuntime = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime();

		// Sanity check
		RENDERER_ASSERT(rendererRuntime.getContext(), nullptr == renderTarget, "The generate mipmaps compositor instance pass needs an invalid render target")

		// Handle texture mipmap generation via custom material blueprint
		const CompositorResourcePassGenerateMipmaps& compositorResourcePassGenerateMipmaps = static_cast<const CompositorResourcePassGenerateMipmaps&>(getCompositorResourcePass());
		RENDERER_ASSERT(rendererRuntime.getContext(), isValid(compositorResourcePassGenerateMipmaps.getTextureAssetId()), "Invalid compositor resource pass generate mipmaps texture asset ID")
		const AssetId materialBlueprintAssetId = compositorResourcePassGenerateMipmaps.getMaterialBlueprintAssetId();
		if (isValid(materialBlueprintAssetId))
		{
			// Sanity check
			RENDERER_ASSERT(rendererRuntime.getContext(), isValid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")

			{ // Record reusable command buffer, if necessary
				const TextureResourceManager& textureResourceManager = rendererRuntime.getTextureResourceManager();
				// TODO(co) "RendererRuntime::TextureResourceManager::getTextureResourceByAssetId()" is considered to be inefficient, don't use it in here
				TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(compositorResourcePassGenerateMipmaps.getTextureAssetId());
				if (nullptr != textureResource)
				{
					Renderer::ITexture* texture = textureResource->getTexturePtr();
					if (nullptr != texture)
					{
						// Sanity check
						RENDERER_ASSERT(rendererRuntime.getContext(), texture->getResourceType() == Renderer::ResourceType::TEXTURE_2D, "The generate mipmaps compositor instance pass needs an 2D texture as texture")

						// Render target size changed?
						Renderer::ITexture2D* texture2D = static_cast<Renderer::ITexture2D*>(texture);
						const uint32_t renderTargetWidth = texture2D->getWidth();
						const uint32_t renderTargetHeight = texture2D->getHeight();
						const uint32_t numberOfMipmaps = Renderer::ITexture::getNumberOfMipmaps(renderTargetWidth, renderTargetHeight);
						if (mRenderTargetWidth != renderTargetWidth || mRenderTargetHeight != renderTargetHeight)
						{
							mRenderTargetWidth = renderTargetWidth;
							mRenderTargetHeight = renderTargetHeight;
							mFramebuffersPtrs.resize(numberOfMipmaps);
							Renderer::IRenderer& renderer = rendererRuntime.getRenderer();
							Renderer::IRenderPass* renderPass = renderer.createRenderPass(0, nullptr, Renderer::TextureFormat::D32_FLOAT);	// TODO(co) Make the texture format flexible, custom mipmap generation also makes sense for color textures
							for (uint32_t mipmapIndex = 1; mipmapIndex < numberOfMipmaps; ++mipmapIndex)
							{
								const Renderer::FramebufferAttachment depthFramebufferAttachment(texture, mipmapIndex, 0);
								mFramebuffersPtrs[mipmapIndex] = renderer.createFramebuffer(*renderPass, nullptr, &depthFramebufferAttachment);
								RENDERER_SET_RESOURCE_DEBUG_NAME(mFramebuffersPtrs[mipmapIndex], ("Compositor instance pass generate mipmap " + std::to_string(mipmapIndex)).c_str())
							}
						}

						// Record reusable command buffer
						// TODO(co) Optimization: Make this hot-reloading ready and also listen to other critical compositor setting changes like number of multisamples, when done we can fill the following command buffer once and then just reuse it
						// TODO(co) There's certainly room for command buffer optimization here (e.g. the graphics pipeline state stays the same)
						mCommandBuffer.clear();
						if (!mFramebuffersPtrs.empty())
						{
							// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
							RENDERER_SCOPED_PROFILER_EVENT_DYNAMIC(rendererRuntime.getContext(), mCommandBuffer, compositorResourcePassGenerateMipmaps.getDebugName())

							// Basing on "Hierarchical-Z map based occlusion culling" - "Hi-Z map construction" - http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/
							uint32_t currentWidth = renderTargetWidth;
							uint32_t currentHeight = renderTargetHeight;
							for (uint32_t mipmapIndex = 1; mipmapIndex < numberOfMipmaps; ++mipmapIndex)
							{
								// Calculate next viewport size and ensure that the viewport size is always at least 1x1
								currentWidth = Renderer::ITexture::getHalfSize(currentWidth);
								currentHeight = Renderer::ITexture::getHalfSize(currentHeight);

								// Set graphics render target
								Renderer::Command::SetGraphicsRenderTarget::create(mCommandBuffer, mFramebuffersPtrs[mipmapIndex]);

								// Set the graphics viewport and scissor rectangle
								Renderer::Command::SetGraphicsViewportAndScissorRectangle::create(mCommandBuffer, 0, 0, currentWidth, currentHeight);

								// Restrict fetches only to previous depth texture mipmap level
								Renderer::Command::SetTextureMinimumMaximumMipmapIndex::create(mCommandBuffer, *texture, mipmapIndex - 1, mipmapIndex - 1);

								// Execute the compute pass
								CompositorContextData localCompositorContextData(compositorContextData.getCompositorWorkspaceInstance(), nullptr);
								mCompositorInstancePassCompute->onFillCommandBuffer(mFramebuffersPtrs[mipmapIndex], localCompositorContextData, mCommandBuffer);
								mCompositorInstancePassCompute->onPostCommandBufferExecution();
							}

							// Reset mipmap level range for the depth texture
							Renderer::Command::SetTextureMinimumMaximumMipmapIndex::create(mCommandBuffer, *texture, 0, numberOfMipmaps - 1);
						}
					}
					else
					{
						// Error!
						RENDERER_ASSERT(rendererRuntime.getContext(), false, "Texture resource has no renderer texture instance")
					}
				}
				else
				{
					// Error!
					RENDERER_ASSERT(rendererRuntime.getContext(), false, "Failed to get texture resource by asset ID")
				}
			}

			// Fill given command buffer, if necessary
			if (!mCommandBuffer.isEmpty())
			{
				mCommandBuffer.submitToCommandBuffer(commandBuffer);
			}
		}
		else
		{
			// Sanity check
			RENDERER_ASSERT(rendererRuntime.getContext(), isInvalid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")

			// Generate mipmaps
			// TODO(co) "RendererRuntime::TextureResourceManager::getTextureResourceByAssetId()" is considered to be inefficient, don't use it in here
			TextureResource* textureResource = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getTextureResourceManager().getTextureResourceByAssetId(compositorResourcePassGenerateMipmaps.getTextureAssetId());
			if (nullptr != textureResource)
			{
				Renderer::ITexture* texture = textureResource->getTexturePtr();
				if (nullptr != texture)
				{
					Renderer::Command::GenerateMipmaps::create(commandBuffer, *texture);
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
			RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), isValid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")

			// Create compositor pass compute
			MaterialProperties materialProperties;
			mCompositorResourcePassCompute = new CompositorResourcePassCompute(compositorResourcePassGenerateMipmaps.getCompositorTarget(), materialBlueprintAssetId, materialProperties);
			#if defined(_DEBUG) || defined(RENDERER_RUNTIME_PROFILER)
				mCompositorResourcePassCompute->setDebugName("Generate mipmap");
			#endif
			mCompositorInstancePassCompute = new CompositorInstancePassCompute(*mCompositorResourcePassCompute, getCompositorNodeInstance());
			getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getMaterialResourceManager().getById(mCompositorInstancePassCompute->getMaterialResourceId()).setPropertyById(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty(), MaterialPropertyValue::fromTextureAssetId(compositorResourcePassGenerateMipmaps.getTextureAssetId()));
		}
		else
		{
			// Sanity check
			RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), isInvalid(compositorResourcePassGenerateMipmaps.getTextureMaterialBlueprintProperty()), "Invalid compositor resource pass generate mipmaps texture material blueprint property")
		}
	}

	CompositorInstancePassGenerateMipmaps::~CompositorInstancePassGenerateMipmaps()
	{
		// Handle texture mipmap generation via custom material blueprint: Destroy compositor pass compute, if there's one
		if (nullptr != mCompositorInstancePassCompute)
		{
			delete mCompositorInstancePassCompute;
			mCompositorInstancePassCompute = nullptr;
			RENDERER_ASSERT(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getContext(), nullptr != mCompositorResourcePassCompute, "Invalid compositor resource pass compute")
			delete mCompositorResourcePassCompute;
			mCompositorResourcePassCompute = nullptr;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
