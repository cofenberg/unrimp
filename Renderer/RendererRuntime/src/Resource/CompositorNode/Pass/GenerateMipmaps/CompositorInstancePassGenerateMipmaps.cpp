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
#include "RendererRuntime/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorInstancePassGenerateMipmaps.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/Quad/CompositorInstancePassQuad.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/Quad/CompositorResourcePassQuad.h"
#include "RendererRuntime/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorContextData.h"
#include "RendererRuntime/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Resource/Texture/TextureResource.h"
#include "RendererRuntime/Core/IProfiler.h"
#include "RendererRuntime/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassGenerateMipmaps::onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		{ // Record reusable command buffer, if necessary
			const IRendererRuntime& rendererRuntime = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime();
			const TextureResourceManager& textureResourceManager = rendererRuntime.getTextureResourceManager();
			const CompositorResourcePassGenerateMipmaps& compositorResourcePassGenerateMipmaps = static_cast<const CompositorResourcePassGenerateMipmaps&>(getCompositorResourcePass());
			// TODO(co) "RendererRuntime::TextureResourceManager::getTextureResourceByAssetId()" is considered to be inefficient, don't use it in here
			TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(compositorResourcePassGenerateMipmaps.getDepthTextureAssetId());
			if (nullptr != textureResource)
			{
				Renderer::ITexture* texture = textureResource->getTexture();
				if (nullptr != texture)
				{
					// Render target size changed?
					uint32_t renderTargetWidth = 0;
					uint32_t renderTargetHeight = 0;
					renderTarget.getWidthAndHeight(renderTargetWidth, renderTargetHeight);
					const uint32_t numberOfMipmaps = Renderer::ITexture::getNumberOfMipmaps(renderTargetWidth, renderTargetHeight);
					if (mRenderTargetWidth != renderTargetWidth || mRenderTargetHeight != renderTargetHeight)
					{
						mRenderTargetWidth = renderTargetWidth;
						mRenderTargetHeight = renderTargetHeight;
						mFramebuffersPtrs.resize(numberOfMipmaps);
						Renderer::IRenderer& renderer = rendererRuntime.getRenderer();
						Renderer::IRenderPass* renderPass = renderer.createRenderPass(0, nullptr, Renderer::TextureFormat::D32_FLOAT);	// TODO(co) Make the texture format flexible
						for (uint32_t mipmapIndex = 1; mipmapIndex < numberOfMipmaps; ++mipmapIndex)
						{
							const Renderer::FramebufferAttachment depthFramebufferAttachment(texture, mipmapIndex, 0);
							mFramebuffersPtrs[mipmapIndex] = renderer.createFramebuffer(*renderPass, nullptr, &depthFramebufferAttachment);
							RENDERER_SET_RESOURCE_DEBUG_NAME(mFramebuffersPtrs[mipmapIndex], ("Compositor instance pass generate mipmap " + std::to_string(mipmapIndex)).c_str())
						}

						// Record reusable command buffer
						// TODO(co) There's certainly room for command buffer optimization here (e.g. the pipeline state stays the same)
						mCommandBuffer.clear();
						if (!mFramebuffersPtrs.empty())
						{
							// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
							RENDERER_SCOPED_PROFILER_EVENT_FUNCTION(rendererRuntime.getContext(), mCommandBuffer)

							// Basing on "Hierarchical-Z map based occlusion culling" - "Hi-Z map construction" - http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/
							uint32_t currentWidth = renderTargetWidth;
							uint32_t currentHeight = renderTargetHeight;
							for (uint32_t mipmapIndex = 1; mipmapIndex < numberOfMipmaps; ++mipmapIndex)
							{
								// Calculate next viewport size and ensure that the viewport size is always at least 1x1
								currentWidth = Renderer::ITexture::getHalfSize(currentWidth);
								currentHeight = Renderer::ITexture::getHalfSize(currentHeight);

								// Set render target
								Renderer::Command::SetRenderTarget::create(mCommandBuffer, mFramebuffersPtrs[mipmapIndex]);

								// Set the viewport and scissor rectangle
								Renderer::Command::SetViewportAndScissorRectangle::create(mCommandBuffer, 0, 0, currentWidth, currentHeight);

								// Restrict fetches only to previous depth texture mipmap level
								Renderer::Command::SetTextureMinimumMaximumMipmapIndex::create(mCommandBuffer, *texture, mipmapIndex - 1, mipmapIndex - 1);

								// Draw full-screen quad
								CompositorContextData localCompositorContextData(compositorContextData.getCompositorWorkspaceInstance(), nullptr);
								mCompositorInstancePassQuad->onFillCommandBuffer(*mFramebuffersPtrs[mipmapIndex], localCompositorContextData, mCommandBuffer);
								mCompositorInstancePassQuad->onPostCommandBufferExecution();
							}

							// Reset mipmap level range for the depth texture
							Renderer::Command::SetTextureMinimumMaximumMipmapIndex::create(mCommandBuffer, *texture, 0, numberOfMipmaps - 1);
						}
					}
				}
				else
				{
					assert(false && "Texture resource has no renderer texture instance");
				}
			}
			else
			{
				assert(false && "Failed to get texture resource by asset ID");
			}
		}

		// Fill given command buffer, if necessary
		if (!mCommandBuffer.isEmpty())
		{
			mCommandBuffer.submitToCommandBuffer(commandBuffer);
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassGenerateMipmaps::CompositorInstancePassGenerateMipmaps(const CompositorResourcePassGenerateMipmaps& compositorResourcePassGenerateMipmaps, const CompositorNodeInstance& compositorNodeInstance) :
		ICompositorInstancePass(compositorResourcePassGenerateMipmaps, compositorNodeInstance),
		mCompositorResourcePassQuad(nullptr),
		mCompositorInstancePassQuad(nullptr),
		mRenderTargetWidth(getUninitialized<uint32_t>()),
		mRenderTargetHeight(getUninitialized<uint32_t>())
	{
		// Create compositor pass quad
		MaterialProperties materialProperties;
		mCompositorResourcePassQuad = new CompositorResourcePassQuad(compositorResourcePassGenerateMipmaps.getCompositorTarget(), compositorResourcePassGenerateMipmaps.getMaterialBlueprintAssetId(), materialProperties);
		mCompositorInstancePassQuad = new CompositorInstancePassQuad(*mCompositorResourcePassQuad, getCompositorNodeInstance());
		// TODO(co) Make this more generic
		getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getMaterialResourceManager().getById(mCompositorInstancePassQuad->getMaterialResourceId()).setPropertyById(STRING_ID("DepthMap"), MaterialPropertyValue::fromTextureAssetId(compositorResourcePassGenerateMipmaps.getDepthTextureAssetId()));
	}

	CompositorInstancePassGenerateMipmaps::~CompositorInstancePassGenerateMipmaps()
	{
		// Destroy compositor pass quad
		delete mCompositorInstancePassQuad;
		mCompositorInstancePassQuad = nullptr;
		delete mCompositorResourcePassQuad;
		mCompositorResourcePassQuad = nullptr;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
